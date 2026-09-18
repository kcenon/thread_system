# Coherence gates

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
