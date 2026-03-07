# SOUP List &mdash; thread_system

> **Software of Unknown Provenance (SOUP) Register per IEC 62304:2006+AMD1:2015 &sect;8.1.2**
>
> This document is the authoritative reference for all external software dependencies.
> Every entry must include: title, manufacturer, unique version identifier, license, and known anomalies.

| Document | Version |
|----------|---------|
| IEC 62304 Reference | &sect;8.1.2 Software items from SOUP |
| Last Reviewed | 2026-03-07 |
| thread_system Version | 0.1.0 |

---

## Production SOUP

| ID | Name | Manufacturer | Version | License | Usage | Safety Class | Linking | Known Anomalies |
|----|------|-------------|---------|---------|-------|-------------|---------|-----------------|
| SOUP-001 | [GNU libiconv](https://www.gnu.org/software/libiconv/) | GNU Project | 1.17 | LGPL-2.1-or-later | Character encoding conversion (wide/narrow strings, non-Windows only) | A | **Dynamic only** (LGPL) | None |

> **LGPL Compliance**: libiconv must be dynamically linked to preserve BSD-3-Clause licensing. macOS provides it as a system framework (always dynamic). Linux glibc includes iconv natively. Windows is excluded (`"platform": "!windows"`).

---

## Optional SOUP

| ID | Name | Manufacturer | Version | License | Usage | Safety Class | Linking | Known Anomalies |
|----|------|-------------|---------|---------|-------|-------------|---------|-----------------|
| SOUP-002 | [spdlog](https://github.com/gabime/spdlog) | Gabi Melman | 1.13.0 | MIT | Fast C++ logging library (optional `logging` feature) | A | Header-only or shared | None |

---

## Development/Test SOUP (Not Deployed)

| ID | Name | Manufacturer | Version | License | Usage | Qualification |
|----|------|-------------|---------|---------|-------|--------------|
| SOUP-T01 | [Google Test](https://github.com/google/googletest) | Google | 1.14.0 | BSD-3-Clause | Unit testing framework (includes GMock) | Required |
| SOUP-T02 | [Google Benchmark](https://github.com/google/benchmark) | Google | 1.8.3 | Apache-2.0 | Performance benchmarking framework | Not required |

---

## Safety Classification Key

| Class | Definition | Example |
|-------|-----------|---------|
| **A** | No contribution to hazardous situation | Logging, formatting, character encoding |
| **B** | Non-serious injury possible | Data processing, network communication |
| **C** | Death or serious injury possible | Encryption, access control |

---

## Version Pinning (IEC 62304 Compliance)

All SOUP versions are pinned in `vcpkg.json` via the `overrides` field:

```json
{
  "overrides": [
    { "name": "libiconv", "version": "1.17" },
    { "name": "spdlog", "version": "1.13.0" },
    { "name": "gtest", "version": "1.14.0" },
    { "name": "benchmark", "version": "1.8.3" }
  ]
}
```

The vcpkg baseline is locked in `vcpkg-configuration.json` to ensure reproducible builds.

---

## Version Update Process

When updating any SOUP dependency:

1. Update the version in `vcpkg.json` (overrides section)
2. Update the corresponding row in this document
3. Verify no new known anomalies (check CVE databases)
4. Run full CI/CD pipeline to confirm compatibility
5. Document the change in the PR description

---

## License Compliance Summary

| License | Count | Copyleft | Obligation |
|---------|-------|----------|------------|
| LGPL-2.1-or-later | 1 | Weak | Dynamic linking required; include license text |
| MIT | 1 | No | Include copyright notice |
| BSD-3-Clause | 1 | No | Include copyright + no-endorsement clause |
| Apache-2.0 | 1 | No | Include license + NOTICE file |

> **LGPL contamination**: Avoided by dynamic linking. libiconv is always loaded as a shared library on supported platforms (macOS system framework, Linux glibc).
