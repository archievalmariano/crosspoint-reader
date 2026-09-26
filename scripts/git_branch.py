"""
PlatformIO pre-build script: inject the firmware identity into CROSSPOINT_VERSION.

Suite builds (those declaring `custom_package`) show
`{version}-A{apps_version}-{PACKAGE}`, e.g. `1.6.0-A1.0.2-GP`: `version` is the
CrossPoint base and `apps_version` this firmware's own release number, from the
[crosspoint] section of platformio.ini. The package also selects which apps are
built (see PACKAGE_APPS). Suite builds get CROSSPOINT_PACKAGE and
CROSSPOINT_OTA_VERSION (plain `apps_version`) for the update check, and
CROSSPOINT_OTA_REPO (`ota_repo`, whose GitHub releases it reads). Other
environments set CROSSPOINT_VERSION in the ini.
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


# Suite environments declare `custom_package` in platformio.ini: one letter per
# app they include (g = GOTO, p = ON POINT). Each letter defines the app's build
# flag; apps left out also have their sources excluded. Environments without a
# package keep every app and get no suite identity, except these, which still
# show the suite version string.
PACKAGE_APPS = {
    'g': ('GOTO_ENABLED', 'activities/goto'),
    'p': ('ON_POINT_ENABLED', 'activities/on_point'),
}
IDENTITY_ONLY_ENVS = ('sticky',)


def get_package(env):
    try:
        package = env.GetProjectOption('custom_package', '')
    except Exception:  # direct run with the fake env below
        package = env.get('custom_package', '')
    package = (package or '').strip().lower()
    unknown = set(package) - set(PACKAGE_APPS)
    if unknown or len(set(package)) != len(package):
        raise ValueError(f'invalid custom_package "{package}" (letters: {"".join(PACKAGE_APPS)})')
    # Canonical letter order, so "pg" and "gp" name the same package.
    return ''.join(letter for letter in PACKAGE_APPS if letter in package)


def apply_package(env, package):
    excluded = []
    for letter, (flag, src_dir) in PACKAGE_APPS.items():
        if not package or letter in package:
            env.Append(CPPDEFINES=[(flag, 1)])
        else:
            excluded.append(f'-<{src_dir}/>')
    if excluded:
        # An empty SRC_FILTER means PlatformIO's default "+<*>"; keep it, or the
        # exclusions alone would drop every source.
        base = env.get('SRC_FILTER') or ['+<*>']
        if isinstance(base, str):
            base = [base]
        env.Replace(SRC_FILTER=list(base) + excluded)


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
    package = get_package(env)
    apply_package(env, package)
    if not package and env['PIOENV'] not in IDENTITY_ONLY_ENVS:
        return

    project_dir = env['PROJECT_DIR']
    base_version = get_crosspoint_option(project_dir, 'version', '0.0.0')
    apps_version = get_crosspoint_option(project_dir, 'apps_version', '0.0.0')

    version_string = f'{base_version}-A{apps_version}'
    if package:
        version_string += f'-{package.upper()}'
    _define(env, 'CROSSPOINT_VERSION', version_string)
    print(f'CrossPoint build version: {version_string}')

    if package:
        _define(env, 'CROSSPOINT_PACKAGE', package)
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
    inject_version(_Env({'PIOENV': 'default', 'PROJECT_DIR': _project_dir,
                         'custom_package': os.environ.get('CUSTOM_PACKAGE', 'gp')}))
