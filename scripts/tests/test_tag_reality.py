import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("tag", Path(__file__).parents[1] / "check_tag_reality.py")
tag = importlib.util.module_from_spec(spec)
spec.loader.exec_module(tag)


class TagRealityTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.git("init", "-q")
        self.git("config", "user.email", "test@example.invalid")
        self.git("config", "user.name", "Fixture")
        self.git("-c", "commit.gpgsign=false", "commit", "--allow-empty", "-qm", "release")
        self.release = self.git("rev-parse", "HEAD")
        self.repo = "kcenon/common_system"
        self.port = self.root / "port"
        self.port.mkdir()
        (self.port / "vcpkg.json").write_text('{"version-semver":"1.2.3"}')
        self.write_port("a" * 128)

    def git(self, *args):
        return subprocess.check_output(["git", "-C", str(self.root), *args], text=True,
                                       stderr=subprocess.PIPE).strip()

    def write_port(self, digest, ref='"v${VERSION}"', repo="kcenon/common_system"):
        (self.port / "portfile.cmake").write_text(f'vcpkg_from_github(REPO {repo}\n REF {ref}\n SHA512 {digest})')

    def test_lightweight_and_annotated_tags(self):
        for annotated in (False, True):
            self.git("-c", "tag.gpgsign=false", "tag", *( ["-a", "-m", "release"] if annotated else []), "v1.2.3")
            self.assertEqual(tag.tag_commit(self.root, "v1.2.3"), self.release)
            self.git("tag", "-d", "v1.2.3")

    def test_missing_and_bad_tag(self):
        for name in ("v1.2.3", "--help", "main", "v1.2.3;echo"):
            with self.assertRaises(ValueError):
                tag.tag_commit(self.root, name)

    def test_wrong_expected_commit_fails_and_later_branch_does_not(self):
        self.git("tag", "v1.2.3")
        self.git("-c", "commit.gpgsign=false", "commit", "--allow-empty", "-qm", "later")
        with self.assertRaisesRegex(ValueError, "expected release merge"):
            tag.verify(self.root, self.repo, "v1.2.3", self.git("rev-parse", "HEAD"), identity_only=True)
        result = tag.verify(self.root, self.repo, "v1.2.3", self.release, identity_only=True)
        self.assertEqual(result["tag_commit"], self.release)
        self.assertEqual(result["stage"], "identity")

    def test_absent_provenance_is_not_a_pass(self):
        with patch.object(tag.subprocess, "check_output", return_value="[]"):
            with self.assertRaisesRegex(ValueError, "provenance"):
                tag.release_provenance(self.root, self.repo, "v1.2.3", self.release)

    def test_record_requires_source(self):
        (self.root / "ci").mkdir()
        p = self.root / "ci/release-provenance.json"
        p.write_text(json.dumps({"v1.2.3": {"merge_commit": self.release}}))
        with self.assertRaises(ValueError):
            tag.release_provenance(self.root, self.repo, "v1.2.3", self.release)

    def test_port_version_ref_repo_and_digest(self):
        url, digest = tag.port_metadata(self.port, self.repo, "v1.2.3")
        self.assertTrue(url.endswith("/refs/tags/v1.2.3.tar.gz"))
        self.assertEqual(digest, "a" * 128)
        for digest, ref, repo in [("0", "v1.2.3", self.repo), ("a"*128, "main", self.repo),
                                  ("a"*128, "v1.2.3", "someone/else")]:
            self.write_port(digest, ref, repo)
            with self.assertRaises(ValueError):
                tag.port_metadata(self.port, self.repo, "v1.2.3")

    def test_stored_hash_is_compared(self):
        self.git("tag", "v1.2.3")
        with patch.object(tag, "archive_hash", return_value="b" * 128):
            with self.assertRaisesRegex(ValueError, "SHA512 mismatch"):
                tag.verify(self.root, self.repo, "v1.2.3", self.release, self.port)

    def test_success_rechecks_remote_tag(self):
        self.git("tag", "v1.2.3")
        original = tag.subprocess.check_output
        def command(args, **kwargs):
            if args[1] == "ls-remote":
                return self.release + "\trefs/tags/v1.2.3\n"
            return original(args, **kwargs)
        with patch.object(tag, "archive_hash", return_value="a"*128), patch.object(tag.subprocess, "check_output", side_effect=command):
            result = tag.verify(self.root, self.repo, "v1.2.3", self.release, self.port)
            self.assertEqual(result["stage"], "identity-and-port")


if __name__ == "__main__":
    unittest.main()
