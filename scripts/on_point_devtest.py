"""PlatformIO pre-script for the DEV-ONLY X4 Pro device-test envs.

`x4pro-all-devtest` (the shipped catalogue) and `x4pro-all-capacity-devtest`
(the synthetic 60-route fixture) are installed by hand with esptool after a
full-flash backup, never from PlatformIO, and can't reflash themselves:
CROSSPOINT_FIRMWARE_UPDATES_DISABLED is required and the update sources are
filtered out (platformio.ini [devtest]). This script refuses any other env and
any upload, and marks the image: version suffix `-DEVTEST`, USB product suffix
`_DEVTEST`, and an update repository that does not exist.
"""

import re
import sys

Import("env")  # noqa: F821  (SCons builtin)

ENV_NAME = env["PIOENV"]  # noqa: F821
if not re.match(r"^x4pro-all-(capacity-)?devtest$", ENV_NAME):
    sys.stderr.write(f"on_point_devtest.py: refusing env {ENV_NAME!r}\n")
    env.Exit(1)  # noqa: F821

UPLOAD_TARGETS = {"upload", "uploadfs", "uploadfsota", "program", "upload-monitor"}
if UPLOAD_TARGETS & set(map(str, COMMAND_LINE_TARGETS)):  # noqa: F821
    sys.stderr.write(f"on_point_devtest.py: {ENV_NAME} images are installed by hand only\n")
    env.Exit(1)  # noqa: F821

if not any(str(f).startswith("-DCROSSPOINT_FIRMWARE_UPDATES_DISABLED") for f in env.get("BUILD_FLAGS", [])):  # noqa: F821
    sys.stderr.write(f"on_point_devtest.py: {ENV_NAME} must define CROSSPOINT_FIRMWARE_UPDATES_DISABLED\n")
    env.Exit(1)  # noqa: F821

# Identity. CROSSPOINT_VERSION / CROSSPOINT_OTA_REPO come from git_branch.py
# (an earlier pre-script) as escaped string defines.
defines = []
for item in env.get("CPPDEFINES", []):  # noqa: F821
    if isinstance(item, (tuple, list)) and len(item) == 2 and item[0] == "CROSSPOINT_VERSION":
        defines.append((item[0], item[1][:-2] + '-DEVTEST\\"'))
    elif isinstance(item, (tuple, list)) and len(item) == 2 and item[0] == "CROSSPOINT_OTA_REPO":
        defines.append((item[0], '\\"invalid/on-point-devtest-build\\"'))
    else:
        defines.append(item)
env.Replace(CPPDEFINES=defines)  # noqa: F821
env.Replace(BUILD_FLAGS=[  # noqa: F821
    str(f)[:-2] + '_DEVTEST\\"' if str(f).startswith("-DUSB_PRODUCT=") else f
    for f in env.get("BUILD_FLAGS", [])  # noqa: F821
])
print(f"ON POINT device test: {ENV_NAME} (firmware updates disabled, never a release)")
