#!/usr/bin/env python3
"""
Log Level Migration Script for thread_system

This script migrates code from the legacy log_level enum to log_level_v2.

Usage:
    python migrate_log_levels.py [options] <path>

Options:
    --dry-run       Show changes without modifying files
    --diff          Show unified diff of changes
    --verbose       Show detailed progress
    --backup        Create .bak backup files before modifying
    --help          Show this help message

Examples:
    # Preview changes for a single file
    python migrate_log_levels.py --dry-run --diff src/my_file.cpp

    # Migrate all files in a directory
    python migrate_log_levels.py --backup src/

    # Preview changes for the entire project
    python migrate_log_levels.py --dry-run --diff .

Migration Rules:
    log_level::trace    -> log_level_v2::trace
    log_level::debug    -> log_level_v2::debug
    log_level::info     -> log_level_v2::info
    log_level::warning  -> log_level_v2::warn    (note: warning -> warn)
    log_level::error    -> log_level_v2::error
    log_level::critical -> log_level_v2::critical

Note: This script does NOT modify logger_interface implementations that must
keep using log_level for interface compatibility during the deprecation period.
"""

import argparse
import os
import re
import sys
from pathlib import Path
from typing import List, Tuple, Optional
import difflib
import shutil


# File extensions to process
SUPPORTED_EXTENSIONS = {'.cpp', '.hpp', '.h', '.cc', '.cxx', '.hxx'}

# Patterns to skip (files that implement logger_interface)
SKIP_PATTERNS = [
    r'logger_interface\.h',
    r'thread_logger\.h',
]

# Migration mappings
LEVEL_MAPPINGS = {
    'trace': 'trace',
    'debug': 'debug',
    'info': 'info',
    'warning': 'warn',  # note: warning -> warn
    'error': 'error',
    'critical': 'critical',
}


class MigrationResult:
    """Result of migrating a single file."""
    def __init__(self, path: str):
        self.path = path
        self.original_content: str = ""
        self.new_content: str = ""
        self.changes: List[Tuple[int, str, str]] = []  # (line_num, old, new)
        self.skipped: bool = False
        self.skip_reason: str = ""
        self.error: Optional[str] = None


def should_skip_file(filepath: str) -> Tuple[bool, str]:
    """Check if a file should be skipped from migration."""
    filename = os.path.basename(filepath)

    for pattern in SKIP_PATTERNS:
        if re.search(pattern, filename):
            return True, f"Matches skip pattern: {pattern}"

    return False, ""


def create_migration_pattern():
    """Create regex pattern for matching log_level usage."""
    # Match log_level::xxx but not log_level_v2::xxx
    # Also match with optional namespace prefix like kcenon::thread::log_level::xxx
    levels = '|'.join(LEVEL_MAPPINGS.keys())
    pattern = rf'(?<!log_level_v2::)(?<!_v2::)\blog_level::({levels})\b'
    return re.compile(pattern)


def migrate_line(line: str, pattern: re.Pattern) -> Tuple[str, bool]:
    """
    Migrate a single line of code.
    Returns (new_line, was_changed).
    """
    def replace_match(match):
        old_level = match.group(1)
        new_level = LEVEL_MAPPINGS.get(old_level, old_level)
        return f'log_level_v2::{new_level}'

    new_line = pattern.sub(replace_match, line)
    return new_line, new_line != line


def needs_include_update(content: str) -> bool:
    """Check if file needs log_level.h include added."""
    # Check if file uses log_level_v2 after migration
    if 'log_level_v2::' in content:
        # Check if it already includes log_level.h
        if re.search(r'#include\s*[<"].*log_level\.h[>"]', content):
            return False
        return True
    return False


def add_log_level_include(content: str) -> str:
    """Add log_level.h include to the file if needed."""
    # Find a good place to insert the include
    # Prefer after other kcenon/thread includes

    lines = content.split('\n')
    insert_index = -1

    for i, line in enumerate(lines):
        # Look for existing thread system includes
        if re.search(r'#include\s*[<"]kcenon/thread/', line):
            insert_index = i + 1
        # Or after any include block
        elif line.startswith('#include') and insert_index == -1:
            insert_index = i + 1

    if insert_index == -1:
        # No includes found, insert at beginning after any header guards
        for i, line in enumerate(lines):
            if line.startswith('#pragma once') or re.match(r'#ifndef\s+\w+_H', line):
                insert_index = i + 1
                break
        if insert_index == -1:
            insert_index = 0

    # Insert the include
    include_line = '#include <kcenon/thread/core/log_level.h>'
    if include_line not in content:
        lines.insert(insert_index, include_line)

    return '\n'.join(lines)


def migrate_file(filepath: str, dry_run: bool = True) -> MigrationResult:
    """Migrate a single file."""
    result = MigrationResult(filepath)

    # Check if file should be skipped
    should_skip, reason = should_skip_file(filepath)
    if should_skip:
        result.skipped = True
        result.skip_reason = reason
        return result

    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            result.original_content = f.read()
    except Exception as e:
        result.error = f"Failed to read file: {e}"
        return result

    pattern = create_migration_pattern()
    lines = result.original_content.split('\n')
    new_lines = []

    for i, line in enumerate(lines):
        new_line, changed = migrate_line(line, pattern)
        new_lines.append(new_line)
        if changed:
            result.changes.append((i + 1, line.strip(), new_line.strip()))

    result.new_content = '\n'.join(new_lines)

    # Check if we need to add include
    if result.changes and needs_include_update(result.new_content):
        result.new_content = add_log_level_include(result.new_content)

    return result


def print_diff(result: MigrationResult):
    """Print unified diff of changes."""
    if not result.changes and result.original_content == result.new_content:
        return

    diff = difflib.unified_diff(
        result.original_content.splitlines(keepends=True),
        result.new_content.splitlines(keepends=True),
        fromfile=f'a/{result.path}',
        tofile=f'b/{result.path}',
    )

    for line in diff:
        if line.startswith('+') and not line.startswith('+++'):
            print(f'\033[32m{line}\033[0m', end='')
        elif line.startswith('-') and not line.startswith('---'):
            print(f'\033[31m{line}\033[0m', end='')
        else:
            print(line, end='')


def find_files(path: str) -> List[str]:
    """Find all supported files in the given path."""
    path = Path(path)

    if path.is_file():
        if path.suffix in SUPPORTED_EXTENSIONS:
            return [str(path)]
        return []

    files = []
    for ext in SUPPORTED_EXTENSIONS:
        files.extend(str(p) for p in path.rglob(f'*{ext}'))

    return sorted(files)


def main():
    parser = argparse.ArgumentParser(
        description='Migrate log_level to log_level_v2 in thread_system code',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument('path', help='File or directory to migrate')
    parser.add_argument('--dry-run', action='store_true',
                        help='Show changes without modifying files')
    parser.add_argument('--diff', action='store_true',
                        help='Show unified diff of changes')
    parser.add_argument('--verbose', action='store_true',
                        help='Show detailed progress')
    parser.add_argument('--backup', action='store_true',
                        help='Create .bak backup files before modifying')

    args = parser.parse_args()

    if not os.path.exists(args.path):
        print(f"Error: Path does not exist: {args.path}", file=sys.stderr)
        sys.exit(1)

    files = find_files(args.path)

    if not files:
        print(f"No supported files found in: {args.path}")
        sys.exit(0)

    if args.verbose:
        print(f"Found {len(files)} file(s) to process")

    total_changes = 0
    files_changed = 0
    files_skipped = 0

    for filepath in files:
        result = migrate_file(filepath, dry_run=args.dry_run)

        if result.error:
            print(f"Error processing {filepath}: {result.error}", file=sys.stderr)
            continue

        if result.skipped:
            files_skipped += 1
            if args.verbose:
                print(f"Skipped: {filepath} ({result.skip_reason})")
            continue

        if not result.changes:
            if args.verbose:
                print(f"No changes: {filepath}")
            continue

        files_changed += 1
        total_changes += len(result.changes)

        if args.diff:
            print_diff(result)
            print()
        elif args.verbose or args.dry_run:
            print(f"{'Would modify' if args.dry_run else 'Modified'}: {filepath}")
            for line_num, old, new in result.changes:
                print(f"  Line {line_num}:")
                print(f"    - {old}")
                print(f"    + {new}")

        if not args.dry_run:
            if args.backup:
                shutil.copy2(filepath, filepath + '.bak')

            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(result.new_content)

    # Summary
    print()
    print("=" * 50)
    print("Migration Summary")
    print("=" * 50)
    print(f"Files processed: {len(files)}")
    print(f"Files changed:   {files_changed}")
    print(f"Files skipped:   {files_skipped}")
    print(f"Total changes:   {total_changes}")

    if args.dry_run and files_changed > 0:
        print()
        print("This was a dry run. No files were modified.")
        print("Run without --dry-run to apply changes.")


if __name__ == '__main__':
    main()
