"""PlatformIO pre-script for the DEV-ONLY ON POINT capacity envs.

`x4pro-all-capacity` (size) and `x4pro-all-capacity-bench` (size + on-device
timing via ON_POINT_BENCH_TIMING, src/activities/on_point/OnPointDeviceBench.cpp) build the largest package with the synthetic 60-route fixture in place
of OnPointScheduleData.cpp. This script refuses any other env, generates the
fixture into the build directory, adds it to the build, and marks the image so it can never pass for a release:
version suffix `-CAPACITY`, USB product `CrossPoint_X4_Pro_CAPACITY`, and an
update repository that does not exist.
"""

import os
import re
import subprocess
import sys

Import("env")  # noqa: F821  (SCons builtin)

ENV_NAME = env["PIOENV"]  # noqa: F821
if not re.match(r"^x4pro-all-capacity(-bench)?$", ENV_NAME):
    sys.stderr.write(f"on_point_capacity_fixture.py: refusing env {ENV_NAME!r}\n")
    env.Exit(1)  # noqa: F821

PROJECT = env["PROJECT_DIR"]  # noqa: F821
FIXTURES = os.path.join(PROJECT, "test", "fixtures", "on_point")
GEN_DIR = os.path.join(env.subst("$BUILD_DIR"), "on_point_capacity_gen")  # noqa: F821
os.makedirs(GEN_DIR, exist_ok=True)

subprocess.check_call([env.subst("$PYTHONEXE"), os.path.join(FIXTURES, "gen_capacity_fixture.py"),  # noqa: F821
                       "--out", os.path.join(GEN_DIR, "OnPointCapacityFixtureData.cpp")])
# Sources added here don't get src/ on their include path; append it (searched
# after every existing path, so nothing already resolved changes).
env.Append(CPPPATH=[os.path.join(PROJECT, "src")])  # noqa: F821
env.BuildSources(os.path.join("$BUILD_DIR", "on_point_capacity_fixture"), GEN_DIR)  # noqa: F821

# Identity. CROSSPOINT_VERSION / CROSSPOINT_OTA_REPO come from git_branch.py
# (an earlier pre-script) as escaped string defines.
defines = []
for item in env.get("CPPDEFINES", []):  # noqa: F821
    if isinstance(item, (tuple, list)) and len(item) == 2 and item[0] == "CROSSPOINT_VERSION":
        defines.append((item[0], item[1][:-2] + '-CAPACITY\\"'))
    elif isinstance(item, (tuple, list)) and len(item) == 2 and item[0] == "CROSSPOINT_OTA_REPO":
        defines.append((item[0], '\\"invalid/on-point-capacity-build\\"'))
    else:
        defines.append(item)
env.Replace(CPPDEFINES=defines)  # noqa: F821
env.Replace(BUILD_FLAGS=[  # noqa: F821
    '-DUSB_PRODUCT=\\"CrossPoint_X4_Pro_CAPACITY\\"' if str(f).startswith("-DUSB_PRODUCT=") else f
    for f in env.get("BUILD_FLAGS", [])  # noqa: F821
])
print(f"ON POINT capacity fixture: {ENV_NAME} (synthetic data, never a release)")
