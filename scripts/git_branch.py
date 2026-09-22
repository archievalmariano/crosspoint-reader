"""
PlatformIO pre-build script: inject git branch and short SHA into
CROSSPOINT_VERSION for development environments.

Results in a version string like:  1.1.0-dev-feat-kosync-xpath-05c6cf8
Release environments are unaffected; they set CROSSPOINT_VERSION in the ini.
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


def get_base_version(project_dir):
    ini_path = os.path.join(project_dir, 'platformio.ini')
    if not os.path.isfile(ini_path):
        warn(f'platformio.ini not found at {ini_path}; base version will be "0.0.0"')
        return '0.0.0'
    config = configparser.ConfigParser()
    config.read(ini_path, encoding='utf-8')
    if not config.has_option('crosspoint', 'version'):
        warn('No [crosspoint] version in platformio.ini; base version will be "0.0.0"')
        return '0.0.0'
    return config.get('crosspoint', 'version')


def get_goto_label(project_dir):
    """Committed public Goto label ([crosspoint] goto_label), or '' if absent.

    This makes the dev build's user-facing identity deterministic without an env
    var, so Settings never falls back to a git branch/hash string.
    """
    ini_path = os.path.join(project_dir, 'platformio.ini')
    if not os.path.isfile(ini_path):
        return ''
    config = configparser.ConfigParser()
    config.read(ini_path, encoding='utf-8')
    if not config.has_option('crosspoint', 'goto_label'):
        return ''
    return _sanitize(config.get('crosspoint', 'goto_label').strip())


def _sanitize(value):
    # Strip characters that would break a C string literal.
    return ''.join(c for c in value if c not in '"\\')


def inject_version(env):
    # Applies to the development environments and to the X4 Pro production env,
    # which shows the deterministic `v{base} · {goto_label}` product identity
    # (e.g. `v1.6.0 · GOTO v1.1.0`). Other release envs set CROSSPOINT_VERSION via
    # build_flags in platformio.ini and are unaffected. The x4pro-gh_release env
    # deliberately OMITS the ini CROSSPOINT_VERSION so this is the single source.
    if env['PIOENV'] not in ('default', 'sticky', 'x4pro-gh_release'):
        return

    project_dir = env['PROJECT_DIR']
    base_version = get_base_version(project_dir)

    # Always compute the git branch/sha for the build-log diagnostic below, so an
    # outside developer can still tie a build to its source — it just never goes
    # into the user-facing version string.
    branch = get_git_branch(project_dir)
    short_sha = get_git_short_sha(project_dir)

    # The user-facing on-device identity. Priority:
    #   1. GOTO_DEV_LABEL env override (other developers' short labels), then
    #   2. the committed [crosspoint] goto_label (Project Goto's public identity,
    #      deterministic with no env var — this is what a release build shows), then
    #   3. only if neither exists, the git branch/sha dev fallback.
    # (1) and (2) render as `v{base} · {label}` so both axes are visible, e.g.
    # `v1.6.0 · GOTO v1.0.0`. The git fallback is never the release identity.
    label = _sanitize(os.environ.get('GOTO_DEV_LABEL', '').strip()) or get_goto_label(project_dir)
    if label:
        version_string = f'v{base_version} · {label}'
    else:
        # Drop the branch-type prefix and cap the branch length so long dev branch
        # names cannot overrun the header slot.
        for prefix in ('feature/', 'fix/', 'refactor/', 'docs/', 'chore/'):
            if branch.startswith(prefix):
                branch = branch[len(prefix):]
                break
        version_string = f'{base_version}-dev-{branch[:16]}-{short_sha}'

    env.Append(CPPDEFINES=[('CROSSPOINT_VERSION', f'\\"{version_string}\\"')])
    print(f'CrossPoint build version: {version_string}')
    # Developer diagnostic (build log only, NOT on-device Settings): source ref.
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
