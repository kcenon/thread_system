from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
HELPER = ROOT / "cmake/template/dependency_options.cmake"
if not HELPER.exists():
    HELPER = ROOT / "cmake/KcenonDependencyOptions.cmake"


class DependencyOptionTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        (self.root / "CMakeLists.txt").write_text('''cmake_minimum_required(VERSION 3.20)
project(alias_test NONE)
include("''' + HELPER.as_posix() + '''")
kcenon_dependency_option(KCENON_WITH_TEST_SYSTEM OLD_WITH_TEST "Test dependency" ON)
file(WRITE "${CMAKE_BINARY_DIR}/effective.txt" "${KCENON_WITH_TEST_SYSTEM};${OLD_WITH_TEST}")
if(REQUIRE_DEP AND NOT KCENON_WITH_TEST_SYSTEM)
  message(FATAL_ERROR "test dependency is required")
endif()
''')

    def configure(self, *flags, expected=0):
        p = subprocess.run(["cmake", "-S", str(self.root), "-B", str(self.root / "build"), *flags],
                           text=True, capture_output=True)
        self.assertEqual(p.returncode, expected, p.stdout + p.stderr)
        return p

    def effective(self):
        return (self.root / "build/effective.txt").read_text()

    def test_default_and_legacy_reconfiguration(self):
        self.configure()
        self.assertEqual(self.effective(), "ON;ON")
        self.configure("-DOLD_WITH_TEST=OFF")
        self.assertEqual(self.effective(), "OFF;OFF")
        self.configure("-DOLD_WITH_TEST=ON")
        self.assertEqual(self.effective(), "ON;ON")

    def test_canonical_off_then_on(self):
        self.configure("-DKCENON_WITH_TEST_SYSTEM=OFF")
        self.assertEqual(self.effective(), "OFF;OFF")
        self.configure("-DKCENON_WITH_TEST_SYSTEM=ON")
        self.assertEqual(self.effective(), "ON;ON")

    def test_canonical_wins_conflict(self):
        p = self.configure("-DKCENON_WITH_TEST_SYSTEM=OFF", "-DOLD_WITH_TEST=ON")
        self.assertIn("takes precedence", p.stderr)
        self.assertEqual(self.effective(), "OFF;OFF")
        self.configure("-DKCENON_WITH_TEST_SYSTEM=ON", "-DOLD_WITH_TEST=OFF")
        self.assertEqual(self.effective(), "ON;ON")

    def test_agreement_and_required_dependency(self):
        self.configure("-DKCENON_WITH_TEST_SYSTEM=OFF", "-DOLD_WITH_TEST=OFF")
        self.assertEqual(self.effective(), "OFF;OFF")
        self.configure("-DREQUIRE_DEP=ON", expected=1)

    def test_child_does_not_change_parent_or_cache(self):
        child = self.root / "child"
        child.mkdir()
        (child / "CMakeLists.txt").write_text('''
kcenon_dependency_option(KCENON_WITH_TEST_SYSTEM CHILD_ALIAS "child" ON)
if(KCENON_WITH_TEST_SYSTEM OR CHILD_ALIAS)
  message(FATAL_ERROR "lost parent input")
endif()
''')
        p = self.root / "CMakeLists.txt"
        p.write_text(p.read_text() + '''
add_subdirectory(child)
if(NOT OLD_WITH_TEST STREQUAL "OFF")
  message(FATAL_ERROR "child changed parent")
endif()
get_property(cached CACHE OLD_WITH_TEST PROPERTY VALUE)
if(NOT cached STREQUAL "ON")
  message(FATAL_ERROR "legacy cache overwritten")
endif()
''')
        self.configure("-DKCENON_WITH_TEST_SYSTEM=OFF", "-DOLD_WITH_TEST=ON")


if __name__ == "__main__":
    unittest.main()
