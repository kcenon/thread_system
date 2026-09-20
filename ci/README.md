# Coherence gates

This default-branch delivery installs the gate and dispatch tooling. The canonical
option migration and source remediation described below are being reviewed on
`ci/701-coherence-gates` for `develop`; this delivery does not change the release
branch's C++ sources or dependency option behavior.

Tracked by [common_system #701](https://github.com/kcenon/common_system/issues/701).

`ci/coherence.json` selects advisory or enforcing policy. The underlying validators
always return their real result. `scripts/run_coherence.py` saves the command,
source SHA, worktree state, stdout/stderr and raw exit code before applying policy.
An advisory workflow success does not mean the gate passed. Reports are uploaded
as workflow artifacts. `--strict` runs the same validator without advisory handling.

Run locally:

```sh
python3 scripts/run_coherence.py conformance
python3 scripts/run_coherence.py version-drift -- --verbose
python3 -m unittest discover -s scripts/tests -p 'test_*.py'
```

Common alone checks `DEPENDENCY_MATRIX.md`; downstream version checks compare
local FetchContent inputs and vcpkg overrides. Invalid manifests and unresolved
pinned versions fail. A report with zero comparisons must state its reviewed
reason; it is not evidence that two versions agreed.

The tag validator requires independently recorded release-merge provenance and
checks the port's stored SHA512 against the actual archive. Use a merged release
PR, a reviewed `ci/release-provenance.json` record, or `--release-commit` from the
release process. It never substitutes today's branch tip for an old release.
Identity-only validation is explicitly labeled and is not checksum validation.

```sh
python3 scripts/check_tag_reality.py --repo kcenon/thread_system --tag v1.2.3 \
  --release-commit FULL_RELEASE_MERGE_SHA \
  --port-dir vcpkg-ports/kcenon-thread-system
```

Select the port metadata revision that describes the artifact being checked.
Development metadata may describe an unreleased version; do not create or move
release tags just to pass a gate. Generated release port hashes are verified
before registry sync; a post-publication check is detection, not release approval.

`ci/coherence-source.json` records the shared source revision and file hashes.
Update shared scripts/helper tests in common first, run their behavior tests,
then propagate identical copies and refresh the manifest in every repository.

New gates remain advisory until remediation has landed and the raw commands
pass on current `develop` HEAD. Promotion is a separate change: remove advisory
handling via policy, verify negative fixtures fail, then add the exact emitted
check names to required-check settings. Preserve existing protections. YAML
changes alone do not apply repository settings.

## Dependency option migration

- `BUILD_WITH_COMMON_SYSTEM` → `KCENON_WITH_COMMON_SYSTEM`.
- `BUILD_WITH_LOGGER_SYSTEM` → `KCENON_WITH_LOGGER_SYSTEM`.

Canonical inputs take precedence with a warning on conflicts. Legacy-only inputs
remain supported, including changing OFF/ON in an existing cache. Defaults and
required-dependency guards are preserved. Effective values are mirrored in normal
CMake scope; parent inputs and existing cache values are not forcibly overwritten.
Remove an explicitly cached canonical choice with `cmake -U KCENON_WITH_<DEP>`
before returning control to a legacy alias. Source compile definitions keep their
existing names for compatibility. Overlay ports pass both spellings while their
release references may predate the shim; they must retain the legacy flag until
the selected published source supports the canonical one.

## Dispatch delivery

Deliver `coherence-receiver.yml` and its shared validation script on the default
`main` branch before common sends `coherence-check-v1`. The receiver validates
an accepted common lock revision and checks out the selected candidate/locked
SHA, then records strict raw results. `notify-ecosystem.yml` runs on trusted
main/develop pushes and uses `ECOSYSTEM_DISPATCH_TOKEN` to notify common and wait
for correlated cross-build and port-audit artifacts. Missing credentials or
receivers remain failures, not successful delivery evidence.

The common-owned [operations guide](https://github.com/kcenon/common_system/blob/ci/701-coherence-gates/ci/ECOSYSTEM_COHERENCE.md)
describes the routes, credential permissions, profiles, lock bootstrap, staged
release boundary and guarded promotion procedure. Refresh shared files from a
complete common checkout, using a reviewed immutable revision:

```sh
python3 ../common_system/scripts/sync_coherence.py --target . --source-revision FULL_COMMON_SHA
python3 ../common_system/scripts/sync_coherence.py --target . --source-revision FULL_COMMON_SHA --check
```

Release sync must reference the reviewed reusable workflow and validator commit.
`tag-reality-mode: advisory` records release identity/hash failures during rollout.
Promote it separately only after real release provenance and port hashes pass.
The release-triggered workflow checks an already published tag; its enforcing
prepublication boundary is the registry sync.
