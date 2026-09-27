"""
PlatformIO pre-build script: build without The Gate Is Open! when its sources
are missing.

Envs that pull in ${gate.build_flags} define GATE_ENABLED and add include paths
under [gate] root, a checkout of the private the-gate-is-open repo. Where that
checkout is absent (e.g. CI on the public fork), drop GATE_ENABLED so the env
builds without Gate instead of failing on a missing header. All Gate code in
src/ is guarded by GATE_ENABLED, so nothing else needs to change.
"""

import os
import sys

Import("env")  # noqa: F821  # pylint: disable=undefined-variable


def warn(msg):
    print(f'WARNING [gate_optional.py]: {msg}', file=sys.stderr)


def gate_root(env):
    try:
        return env.GetProjectConfig().get('gate', 'root')
    except Exception:  # pylint: disable=broad-exception-caught
        return ''


def is_gate_flag(flag):
    return str(flag).strip().startswith('-DGATE_ENABLED')


# Pre scripts run before build_flags become CPPDEFINES, so filter the raw flags.
flags = env.get('BUILD_FLAGS', [])  # noqa: F821
if any(is_gate_flag(f) for f in flags):
    root = gate_root(env)  # noqa: F821
    if not root or not os.path.isdir(os.path.join(root, 'engine', 'include')):
        warn(f'Gate sources not found at "{root}"; building {env["PIOENV"]} without Gate')  # noqa: F821
        env.Replace(BUILD_FLAGS=[f for f in flags if not is_gate_flag(f)])  # noqa: F821
