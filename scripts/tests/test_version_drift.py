import contextlib
import importlib.util
import io
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("drift", Path(__file__).parents[1] / "check_version_drift.py")
drift = importlib.util.module_from_spec(spec)
spec.loader.exec_module(drift)


class VersionDriftTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.manifest = self.root / "vcpkg.json"
        self.manifest.write_text(json.dumps({"overrides": [{"name": "gtest", "version": "1.17.0"}]}))

    def source(self, content):
        (self.root / "CMakeLists.txt").write_text(content)

    def run_check(self):
        return drift.check(self.root, self.manifest)

    def test_alias_variable_and_comment(self):
        self.source('''# FetchContent_Declare(googletest GIT_TAG v0.0.0)
set(TEST_TAG "v1.17.0" CACHE STRING "tag (pinned)")
FetchContent_Declare(googletest GIT_TAG "${TEST_TAG}")''')
        result = self.run_check()
        self.assertEqual(len(result["compared"]), 1)
        self.assertEqual(result["findings"], [])

    def test_mismatch(self):
        self.source('FetchContent_Declare(googletest GIT_TAG release-1.12.1)')
        self.assertIn("DRIFT gtest", self.run_check()["findings"][0])

    def test_unknown_and_ambiguous_variables_fail(self):
        for prefix in ["", 'set(TAG v1.17.0)\nset(TAG v1.14.0)\n']:
            self.source(prefix + 'FetchContent_Declare(googletest GIT_TAG ${TAG})')
            self.assertEqual(len(self.run_check()["unresolved"]), 1)

    def test_git_commit_is_unresolved_not_a_version(self):
        self.source('FetchContent_Declare(googletest GIT_TAG ' + 'a' * 40 + ')')
        self.assertEqual(len(self.run_check()["unresolved"]), 1)

    def test_archive_and_asio_tags(self):
        self.source('FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz)')
        self.assertEqual(self.run_check()["findings"], [])
        self.assertEqual(drift.normalize_version("asio-1-30-2"), "1.30.2")
        self.assertEqual(drift.normalize_version("sqlite-amalgamation-3450300"), "3.45.3")

    def test_invalid_manifests(self):
        for text in ["{bad", "[]", '{"overrides": {}}', '{"overrides":[{}]}',
                     '{"overrides":[{"name":"gtest","version":"1"},{"name":"gtest","version":"2"}]}']:
            self.manifest.write_text(text)
            with self.assertRaises(ValueError):
                self.run_check()
        self.manifest.unlink()
        with self.assertRaises(ValueError):
            self.run_check()

    def test_no_comparisons_requires_reason(self):
        self.source("project(example)")
        with self.assertRaisesRegex(ValueError, "no comparable"):
            self.run_check()
        result = drift.check(self.root, self.manifest, no_comparisons_reason="Uses installed packages only")
        self.assertEqual(result["compared"], [])
        self.assertTrue(result["no_comparisons_reason"])

    def test_unpinned_declaration_reported(self):
        self.source('FetchContent_Declare(googletest GIT_TAG v1.17.0)\nFetchContent_Declare(catch2 GIT_TAG v3.4.0)')
        self.assertEqual(self.run_check()["unmatched"][0]["dep"], "catch2")

    def test_generated_build_files_excluded(self):
        self.source('FetchContent_Declare(googletest GIT_TAG v1.17.0)')
        p = self.root / "build-debug/_deps/bad/CMakeLists.txt"
        p.parent.mkdir(parents=True)
        p.write_text('FetchContent_Declare(googletest GIT_TAG v0.0.0)')
        self.assertEqual(len(self.run_check()["compared"]), 1)

    def test_matrix_required_and_checked(self):
        matrix = self.root / "MATRIX.md"
        matrix.write_text('| Dependency | Ecosystem Standard |\n|---|---|\n| GTest | **1.14.0** |\n')
        findings, rows = drift.check_matrix_consistency({"gtest": "1.17.0"}, matrix)
        self.assertEqual(rows, 1)
        self.assertEqual(len(findings), 1)
        matrix.write_text("not a matrix")
        with self.assertRaises(ValueError):
            drift.check_matrix_consistency({"gtest": "1.17.0"}, matrix)

    def test_cli_exit_codes_and_json(self):
        report = self.root / "report.json"
        args = ["--project-root", str(self.root), "--no-matrix", "--json-output", str(report)]
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            self.source('FetchContent_Declare(googletest GIT_TAG v1.17.0)')
            self.assertEqual(drift.main(args), 0)
            self.assertEqual(len(json.loads(report.read_text())["compared"]), 1)
            self.source('FetchContent_Declare(googletest GIT_TAG v1.14.0)')
            self.assertEqual(drift.main(args), 1)
            self.manifest.unlink()
            self.assertEqual(drift.main(args), 2)


if __name__ == "__main__":
    unittest.main()
