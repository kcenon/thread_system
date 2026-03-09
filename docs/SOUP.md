# SOUP List &mdash; thread_system

> **Software of Unknown Provenance (SOUP) Register per IEC 62304:2006+AMD1:2015 &sect;8.1.2**
>
> This document is the authoritative reference for all external software dependencies.
> Every entry must include: title, manufacturer, unique version identifier, license, and known anomalies.

| Document | Version |
|----------|---------|
| IEC 62304 Reference | &sect;8.1.2 Software items from SOUP |
| Last Reviewed | 2026-03-10 |
| thread_system Version | 0.1.0 |

---

## Production SOUP

| ID | Name | Manufacturer | Version | License | Usage | Safety Class | Linking | Known Anomalies |
|----|------|-------------|---------|---------|-------|-------------|---------|-----------------|
| SOUP-001 | [simdutf](https://github.com/simdutf/simdutf) | simdutf contributors | 5.2.5 | MIT OR Apache-2.0 | SIMD-accelerated Unicode transcoding (UTF-8/16/32, all platforms) | A | Static or dynamic | None |

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
    { "name": "simdutf", "version": "5.2.5" },
    { "name": "spdlog", "version": "1.13.0" },
    { "name": "gtest", "version": "1.14.0" },
    { "name": "benchmark", "version": "1.8.3" }
  ]
}
```

The vcpkg baseline is locked in `vcpkg-configuration.json` to ensure reproducible builds.

---

## Resolved Provenance Artifacts

The SBOM workflow now performs a minimal CMake configure and publishes the resolved
`simdutf` provenance alongside the CycloneDX/SPDX outputs:

- `build/sbom-provenance/dependency-provenance/simdutf-provenance.json`
- `build/sbom-provenance/dependency-provenance/simdutf-provenance.md`

Interpret `resolved_source` as follows:

- `system`: a pre-installed or OS package manager provided `simdutf` CMake package was used
- `vcpkg`: the resolved target came from the active vcpkg toolchain / `vcpkg_installed`
- `FetchContent`: CMake cloned `https://github.com/simdutf/simdutf.git` during configure

When `resolved_source` is `FetchContent`, verify `resolved_version` and
`source_reference` against the pinned tag `v5.2.5`. The `resolved_path` field shows
the concrete package or source directory that satisfied the dependency in that CI run.

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
| MIT OR Apache-2.0 | 1 | No | Include copyright notice |
| MIT | 1 | No | Include copyright notice |
| BSD-3-Clause | 1 | No | Include copyright + no-endorsement clause |
| Apache-2.0 | 1 | No | Include license + NOTICE file |

> **No LGPL dependencies**: All production dependencies use permissive licenses.
> Static or dynamic linking is permitted without restriction.
