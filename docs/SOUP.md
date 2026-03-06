# SOUP List &mdash; thread_system

> **Software of Unknown Provenance (SOUP) Register per IEC 62304:2006+AMD1:2015 &sect;8.1.2**
>
> This document is the authoritative reference for all external software dependencies.
> Every entry must include: title, manufacturer, unique version identifier, license, and known anomalies.

| Document | Version |
|----------|---------|
| IEC 62304 Reference | &sect;8.1.2 Software items from SOUP |
| Last Reviewed | 2026-03-06 |
| thread_system Version | 3.0.0 |

---

## Internal Ecosystem Dependencies

| ID | Name | Manufacturer | Version | License | Usage | Safety Class | Known Anomalies |
|----|------|-------------|---------|---------|-------|-------------|-----------------|
| INT-001 | [common_system](https://github.com/kcenon/common_system) | kcenon | Latest (source) | BSD-3-Clause | Result&lt;T&gt; pattern, error handling primitives, interfaces | B | None |

> **Note**: common_system is integrated as source-level dependency via CMake `find_package` or adjacent directory detection.

---

## Production SOUP

| ID | Name | Manufacturer | Version | License | Usage | Safety Class | Known Anomalies |
|----|------|-------------|---------|---------|-------|-------------|-----------------|
| SOUP-001 | [libiconv](https://www.gnu.org/software/libiconv/) | GNU Project | 1.17 | LGPL-2.1 | Character encoding conversion (non-Windows platforms only) | A | Dynamic linking required for LGPL compliance |

---

## Optional SOUP

| ID | Name | Manufacturer | Version | License | Usage | Safety Class | Known Anomalies |
|----|------|-------------|---------|---------|-------|-------------|-----------------|
| SOUP-002 | [spdlog](https://github.com/gabime/spdlog) | Gabi Melman | 1.13.0 | MIT | Advanced logging capabilities (optional `logging` feature) | A | None |

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
| **A** | No contribution to hazardous situation | Logging, formatting, test frameworks |
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
| LGPL-2.1 | 1 | Weak | Dynamic linking required |
| MIT | 1 | No | Include copyright notice |
| BSD-3-Clause | 1 | No | Include copyright + no-endorsement clause |
| Apache-2.0 | 1 | No | Include license + NOTICE file |

> **GPL contamination**: None detected. All dependencies are permissively licensed.
> **LGPL note**: libiconv (SOUP-001) uses LGPL-2.1; dynamic linking ensures compliance.
