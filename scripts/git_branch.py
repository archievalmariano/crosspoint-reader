"""
PlatformIO pre-build script: inject the firmware identity into CROSSPOINT_VERSION.

The suite builds show `{version}-A{apps_version}` (e.g. `1.6.0-A1.0.0`) from
the [crosspoint] section of platformio.ini:
`version` is the CrossPoint base, `apps_version` this firmware's own release
number. Suite builds also get CROSSPOINT_OTA_VERSION (plain `apps_version`, for
the update check) and CROSSPOINT_OTA_REPO (`ota_repo`, whose GitHub releases the
update check reads). Other environments set CROSSPOINT_VERSION in the ini.
"""

import configparser
import os
import subprocess
import sys


def warn(msg):
    print(f'WARNING [git_branch.py]: {msg}', file=sys.stderr)


def run_git_value(project_dir, args, label):
    try:
        value = subprocess.check_output(
            ['git', *args],
            text=True, stderr=subprocess.PIPE, cwd=project_dir
        ).strip()
        # Strip characters that would break a C string literal
        return ''.join(c for c in value if c not in '"\\')
    except FileNotFoundError:
        warn(f'git not found on PATH; {label} suffix will be "unknown"')
        return 'unknown'
    except subprocess.CalledProcessError as e:
        warn(
            f'git command failed (exit {e.returncode}): '
            f'{e.stderr.strip()}; {label} suffix will be "unknown"'
        )
        return 'unknown'
    except OSError as e:
        warn(
            f'OS error reading git {label}: {e}; '
            f'{label} suffix will be "unknown"'
        )
        return 'unknown'
    except Exception as e:  # pylint: disable=broad-exception-caught
        warn(
            f'Unexpected error reading git {label}: {e}; '
            f'{label} suffix will be "unknown"'
        )
        return 'unknown'


def get_git_branch(project_dir):
    branch = run_git_value(
        project_dir, ['rev-parse', '--abbrev-ref', 'HEAD'], 'branch'
    )
    # Detached HEAD has no branch name.
    if branch == 'HEAD':
        return 'detached'
    return branch


def get_git_short_sha(project_dir):
    return run_git_value(
        project_dir, ['rev-parse', '--short', 'HEAD'], 'short SHA'
    )


# Environments whose version string comes from this script. The suite
# environments also get the OTA version/repo defines.
SUITE_ENVS = ('default', 'gh_release', 'x4pro', 'x4pro-gh_release')
IDENTITY_ENVS = SUITE_ENVS + ('sticky',)


def get_crosspoint_option(project_dir, key, default):
    ini_path = os.path.join(project_dir, 'platformio.ini')
    if not os.path.isfile(ini_path):
        warn(f'platformio.ini not found at {ini_path}; {key} will be "{default}"')
        return default
    config = configparser.ConfigParser()
    config.read(ini_path, encoding='utf-8')
    if not config.has_option('crosspoint', key):
        warn(f'No [crosspoint] {key} in platformio.ini; it will be "{default}"')
        return default
    return _sanitize(config.get('crosspoint', key).strip())


def _sanitize(value):
    # Strip characters that would break a C string literal.
    return ''.join(c for c in value if c not in '"\\')


def _define(env, name, value):
    env.Append(CPPDEFINES=[(name, f'\\"{value}\\"')])


def inject_version(env):
    if env['PIOENV'] not in IDENTITY_ENVS:
        return

    project_dir = env['PROJECT_DIR']
    base_version = get_crosspoint_option(project_dir, 'version', '0.0.0')
    apps_version = get_crosspoint_option(project_dir, 'apps_version', '0.0.0')

    version_string = f'{base_version}-A{apps_version}'
    _define(env, 'CROSSPOINT_VERSION', version_string)
    print(f'CrossPoint build version: {version_string}')

    if env['PIOENV'] in SUITE_ENVS:
        _define(env, 'CROSSPOINT_OTA_VERSION', apps_version)
        ota_repo = get_crosspoint_option(project_dir, 'ota_repo', '')
        if ota_repo:  # otherwise OtaUpdater keeps its upstream default
            _define(env, 'CROSSPOINT_OTA_REPO', ota_repo)

    # Developer diagnostic (build log only, never on-device): source ref.
    branch = get_git_branch(project_dir)
    short_sha = get_git_short_sha(project_dir)
    print(f'CrossPoint build source ref: {branch}@{short_sha}')


# PlatformIO/SCons entry point — Import and env are SCons builtins injected at runtime.
# When run directly with Python (e.g. for validation), a lightweight fake env is used
# so the git/version logic can be exercised without a full build.
try:
    Import('env')           # noqa: F821  # type: ignore[name-defined]
    inject_version(env)     # noqa: F821  # type: ignore[name-defined]
except NameError:
    class _Env(dict):
        def Append(self, **_): pass

    _project_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    inject_version(_Env({'PIOENV': 'default', 'PROJECT_DIR': _project_dir}))
