import contextlib
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("gate", Path(__file__).parents[1] / "run_coherence.py")
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)


class GatePolicyTests(unittest.TestCase):
    def test_advisory_retains_failure_enforcing_and_strict_fail(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "ci").mkdir()
            (root / "scripts").mkdir()
            (root / "scripts/conformance_lint.py").write_text('print("fixture violation")\nraise SystemExit(1)\n')
            config = root / "ci/coherence.json"
            report = root / "result.json"
            subprocess.run(["git", "init", "-q", str(root)], check=True)
            subprocess.run(["git", "-C", str(root), "-c", "user.name=Fixture", "-c", "user.email=test@example.invalid",
                            "-c", "commit.gpgsign=false", "commit", "--allow-empty", "-qm", "fixture"], check=True)
            for mode, strict, expected in [("advisory", False, 0), ("advisory", True, 1), ("enforcing", False, 1)]:
                config.write_text(json.dumps({"schema_version": 1, "modes": {"conformance": mode}}))
                with contextlib.redirect_stdout(io.StringIO()):
                    self.assertEqual(gate.run_check(root, "conformance", report, strict=strict), expected)
                raw = json.loads(report.read_text())
                self.assertEqual(raw["exit_code"], 1)
                self.assertFalse(raw["passed"])
                self.assertTrue(raw["worktree_changes"])
                self.assertIn("fixture violation", raw["stdout"])
            # Historical source tags need neither the new policy file nor the
            # new validator. The trusted release workflow pins both separately.
            config.unlink()
            trusted = root / "reviewed-tools"
            (trusted / "scripts").mkdir(parents=True)
            (trusted / "scripts/conformance_lint.py").write_text('raise SystemExit(3)\n')
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(gate.run_check(root,"conformance",report,mode_override="advisory",script_root=trusted),0)
                self.assertEqual(gate.run_check(root,"conformance",report,mode_override="enforcing",script_root=trusted),1)
            self.assertEqual(json.loads(report.read_text())["exit_code"],3)


if __name__ == "__main__":
    unittest.main()
