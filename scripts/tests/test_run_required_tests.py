"""Exercise the actual CTest gate with failing/empty/disabled fixtures."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

GATE = Path(__file__).resolve().parents[1] / "run_required_tests.py"


class RequiredTestsGate(unittest.TestCase):
    def check_gate(self, test_command, *, testing="ON", include=".*", required="probe", success=False, gtest=False):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "CMakeLists.txt").write_text(
                "cmake_minimum_required(VERSION 3.20)\nproject(gate NONE)\n"
                "include(CTest)\n" + test_command)
            subprocess.run(["cmake", "-S", tmp, "-B", str(root / "build"),
                            f"-DBUILD_TESTING={testing}"], check=True, capture_output=True)
            (root / "empty-gtest.cmake").write_text(
                'string(REGEX REPLACE "^xml:" "" output "$ENV{GTEST_OUTPUT}")\n'
                'file(MAKE_DIRECTORY "${output}")\n'
                'file(WRITE "${output}/cmake.xml" "<testsuites tests=\\\"0\\\"/>")\n')
            result = subprocess.run([sys.executable, str(GATE), str(root / "build"),
                                     "--include", include, "--require", required, *(["--gtest"] if gtest else [])],
                                    text=True, capture_output=True)
            print(result.stdout + result.stderr)
            self.assertEqual(result.returncode == 0, success)

    def test_passing_test(self):
        self.check_gate('add_test(NAME probe COMMAND "${CMAKE_COMMAND}" -E true)\n', success=True)

    def test_failing_test(self):
        self.check_gate('add_test(NAME probe COMMAND "${CMAKE_COMMAND}" -E false)\n')

    def test_empty_discovery(self):
        self.check_gate("")

    def test_empty_selection(self):
        self.check_gate('add_test(NAME probe COMMAND "${CMAKE_COMMAND}" -E true)\n', include="absent")

    def test_missing_required_test(self):
        self.check_gate('add_test(NAME probe COMMAND "${CMAKE_COMMAND}" -E true)\n', required="missing")

    def test_missing_gtest_execution(self):
        self.check_gate('add_test(NAME probe COMMAND "${CMAKE_COMMAND}" -E true)\n', gtest=True)

    def test_empty_gtest_execution(self):
        self.check_gate('add_test(NAME probe COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_SOURCE_DIR}/empty-gtest.cmake")\n', gtest=True)

    def test_disabled_testing(self):
        self.check_gate('enable_testing()\nadd_test(NAME probe COMMAND "${CMAKE_COMMAND}" -E true)\n', testing="OFF")


if __name__ == "__main__":
    unittest.main()
