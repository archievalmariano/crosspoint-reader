"""Tests for check_no_capacity_fixture.py: each leak path must be caught."""

import os
import sys
import tempfile
import textwrap
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import check_no_capacity_fixture as guard  # noqa: E402

GOOD = """
[base]
build_flags = -DFOO=1
[env:x4pro-all-gh_release]
extends = base
build_flags = ${base.build_flags}
[env:x4pro-all-capacity]
extends = env:x4pro-all-gh_release
build_src_filter = +<*> -<activities/on_point/OnPointScheduleData.cpp>
upload_protocol = custom
upload_command = echo "never flashed" && exit 1
build_flags = ${env:x4pro-all-gh_release.build_flags}
  -DON_POINT_CAPACITY_FIXTURE=1
extra_scripts = pre:scripts/on_point_capacity_fixture.py
[env:x4pro-all-capacity-bench]
extends = env:x4pro-all-capacity
build_flags = ${env:x4pro-all-capacity.build_flags}
  -DON_POINT_BENCH_TIMING=1
[devtest]
build_src_filter =
  -<network/OtaUpdater.cpp>
  -<network/FirmwareFlasher.cpp>
  -<network/OtaBootSwitch.cpp>
  -<network/FirmwareBoardTag.cpp>
  -<activities/settings/OtaUpdateActivity.cpp>
  -<activities/settings/SdFirmwareUpdateActivity.cpp>
[env:x4pro-all-devtest]
extends = env:x4pro-all-gh_release
build_src_filter = +<*> ${devtest.build_src_filter}
upload_protocol = custom
upload_command = echo "by hand only" && exit 1
build_flags = ${env:x4pro-all-gh_release.build_flags}
  -DON_POINT_BENCH_TIMING=1
  -DCROSSPOINT_FIRMWARE_UPDATES_DISABLED=1
extra_scripts = pre:scripts/on_point_devtest.py
[env:x4pro-all-capacity-devtest]
extends = env:x4pro-all-capacity-bench
build_src_filter = ${env:x4pro-all-capacity.build_src_filter} ${devtest.build_src_filter}
build_flags = ${env:x4pro-all-capacity-bench.build_flags}
  -DCROSSPOINT_FIRMWARE_UPDATES_DISABLED=1
extra_scripts = ${env:x4pro-all-capacity.extra_scripts}
  pre:scripts/on_point_devtest.py
"""

WORKFLOW_OK = "matrix:\n  include:\n    - env: x4pro-all-gh_release\n      package: GPD\n"


def run(config, workflow=WORKFLOW_OK):
    with tempfile.TemporaryDirectory() as d:
        ini = os.path.join(d, "platformio.ini")
        wf = os.path.join(d, "release_candidate.yml")
        with open(ini, "w") as f:
            f.write(textwrap.dedent(config))
        with open(wf, "w") as f:
            f.write(workflow)
        return guard.check_config(ini, wf)


class ConfigGuard(unittest.TestCase):
    def test_repository_layout_passes(self):
        self.assertEqual(run(GOOD), [])

    def test_fixture_flag_in_a_release_env_fails(self):
        bad = GOOD.replace("[env:x4pro-all-gh_release]\nextends = base\nbuild_flags = ${base.build_flags}",
                           "[env:x4pro-all-gh_release]\nextends = base\nbuild_flags = ${base.build_flags}\n"
                           "  -DON_POINT_CAPACITY_FIXTURE=1")
        self.assertTrue(any("x4pro-all-gh_release" in p for p in run(bad)))

    def test_bench_flag_in_base_fails(self):
        bad = GOOD.replace("build_flags = -DFOO=1", "build_flags = -DFOO=1 -DON_POINT_BENCH_TIMING=1")
        self.assertTrue(any("[base]" in p for p in run(bad)))

    def test_fixture_sources_in_a_release_filter_fail(self):
        bad = GOOD + "[env:x4pro-gh_release]\nbuild_src_filter = +<*> +<../test/fixtures/on_point/>\n"
        self.assertTrue(any("x4pro-gh_release" in p for p in run(bad)))

    def test_capacity_script_in_a_release_env_fails(self):
        bad = GOOD + "[env:x4pro-gh_release]\nextra_scripts = pre:scripts/on_point_capacity_fixture.py\n"
        self.assertTrue(run(bad))

    def test_release_env_extending_a_capacity_env_fails(self):
        bad = GOOD + "[env:x4pro-sneaky-gh_release]\nextends = env:x4pro-all-capacity\n"
        self.assertTrue(any("extends dev env env:x4pro-all-capacity" in p for p in run(bad)))

    def test_capacity_env_in_the_candidate_matrix_fails(self):
        wf = WORKFLOW_OK + "    - env: x4pro-all-capacity\n      package: GPD\n"
        self.assertTrue(any("release candidate matrix" in p for p in run(GOOD, wf)))

    def test_capacity_env_that_can_upload_fails(self):
        bad = GOOD.replace('upload_command = echo "never flashed" && exit 1\n', "")
        problems = run(bad)
        self.assertTrue(any("must refuse uploads" in p for p in problems), problems)

    def test_devtest_env_without_the_updates_define_fails(self):
        bad = GOOD.replace("  -DON_POINT_BENCH_TIMING=1\n  -DCROSSPOINT_FIRMWARE_UPDATES_DISABLED=1\n",
                           "  -DON_POINT_BENCH_TIMING=1\n")
        self.assertTrue(any("x4pro-all-devtest] must define" in p for p in run(bad)), run(bad))

    def test_capacity_devtest_inherits_nothing_for_the_define(self):
        bad = GOOD.replace("build_flags = ${env:x4pro-all-capacity-bench.build_flags}\n"
                           "  -DCROSSPOINT_FIRMWARE_UPDATES_DISABLED=1\n", "")
        self.assertTrue(any("x4pro-all-capacity-devtest] must define" in p for p in run(bad)), run(bad))

    def test_devtest_env_building_an_update_source_fails(self):
        bad = GOOD.replace("  -<network/FirmwareFlasher.cpp>\n", "")
        problems = run(bad)
        self.assertEqual(sum("must filter out network/FirmwareFlasher.cpp" in p for p in problems), 2, problems)

    def test_devtest_env_without_its_script_fails(self):
        bad = GOOD.replace("extra_scripts = pre:scripts/on_point_devtest.py\n", "")
        self.assertTrue(any("must run scripts/on_point_devtest.py" in p for p in run(bad)))

    def test_capacity_devtest_that_can_upload_fails(self):
        bad = GOOD.replace('upload_command = echo "never flashed" && exit 1\n', 'upload_command = esptool.py\n')
        problems = run(bad)
        self.assertTrue(any("x4pro-all-capacity-devtest] must refuse uploads" in p for p in problems), problems)

    def test_updates_define_in_a_release_env_fails(self):
        bad = GOOD.replace("build_flags = -DFOO=1", "build_flags = -DFOO=1 -DCROSSPOINT_FIRMWARE_UPDATES_DISABLED=1")
        self.assertTrue(any("[base]" in p for p in run(bad)))

    def test_devtest_filter_in_a_release_env_fails(self):
        bad = GOOD + "[env:x4pro-gh_release]\nbuild_src_filter = +<*> ${devtest.build_src_filter}\n"
        self.assertTrue(any("devtest source filter outside" in p for p in run(bad)))

    def test_release_env_extending_a_devtest_env_fails(self):
        bad = GOOD + "[env:x4pro-sneaky-gh_release]\nextends = env:x4pro-all-devtest\n"
        self.assertTrue(any("extends dev env" in p for p in run(bad)))

    def test_devtest_env_in_the_candidate_matrix_fails(self):
        wf = WORKFLOW_OK + "    - env: x4pro-all-devtest\n      package: GPD\n"
        self.assertTrue(any("release candidate matrix" in p for p in run(GOOD, wf)))

    def test_missing_devtest_env_is_reported(self):
        stripped = GOOD.split("[devtest]")[0]
        self.assertTrue(any("devtest rules would be vacuous" in p for p in run(stripped)))

    def test_missing_capacity_env_is_reported(self):
        stripped = GOOD.split("[env:x4pro-all-capacity]")[0]
        self.assertTrue(any("vacuous" in p for p in run(stripped)))


class ArtifactScan(unittest.TestCase):
    def scan(self, payload):
        with tempfile.NamedTemporaryFile(delete=False) as f:
            f.write(payload)
        try:
            return guard.scan([f.name])
        finally:
            os.unlink(f.name)

    def test_clean_image_passes(self):
        self.assertEqual(self.scan(b"\x00CrossPoint 1.6.5-A1.2.0-GPD\x00p2p-naia-pitx\x00"), [])

    def test_each_marker_is_caught(self):
        for marker in (b"fixture-007-xxx", b"FIXTURE ORIGIN 001", b"1.6.5-A1.2.0-GPD-CAPACITY", b"ON_POINT_CAPACITY",
                       b"1.6.5-A1.2.0-GPD-DEVTEST", b"CrossPoint_X4_Pro_DEVTEST"):
            self.assertTrue(self.scan(b"\x00" + marker + b"\x00"), marker)


class DevtestScan(unittest.TestCase):
    def scan(self, payload, suffix=".bin"):
        with tempfile.NamedTemporaryFile(delete=False, suffix=suffix) as f:
            f.write(payload)
        try:
            return guard.scan_devtest([f.name])
        finally:
            os.unlink(f.name)

    def test_devtest_image_passes(self):
        self.assertEqual(self.scan(b"\x00CrossPoint 1.6.5-A1.2.0-GPD-DEVTEST\x00"), [])

    def test_missing_identity_fails(self):
        self.assertTrue(any("no -DEVTEST" in p for p in self.scan(b"\x00CrossPoint 1.6.5-A1.2.0-GPD\x00")))

    def test_update_url_fails(self):
        payload = b"\x001.6.5-DEVTEST\x00https://api.github.com/repos/x/y/releases/latest\x00"
        self.assertEqual(len(self.scan(payload)), 2)

    def test_elf_without_a_symbol_table_fails(self):
        self.assertTrue(any("no symbol table" in p for p in self.scan(b"\x001.6.5-DEVTEST\x00", ".elf")))


if __name__ == "__main__":
    unittest.main()
