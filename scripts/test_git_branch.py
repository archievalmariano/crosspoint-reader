"""Tests for scripts/git_branch.py (package letters) and the envs in platformio.ini.

    python3 -m unittest scripts/test_git_branch.py

DOGEAR (`d`) is opt-in: an env gets it only by listing `d` in its own
custom_package, never from the "no package" fallback that keeps GOTO and ON POINT.
"""
import configparser
import contextlib
import importlib.util
import io
import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def load_git_branch():
    spec = importlib.util.spec_from_file_location("git_branch_under_test", ROOT / "scripts" / "git_branch.py")
    module = importlib.util.module_from_spec(spec)
    with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
        spec.loader.exec_module(module)  # runs its validation stub once (package "gp")
    module.run_git_value = lambda *_: "test"
    return module


gb = load_git_branch()
WOLF = ("-DWOLFSSL_SHA384", "-DWOLFSSL_SP_384")
APP_DIRS = {"g": "activities/goto", "p": "activities/on_point", "d": "activities/dogear"}
FLAGS = {"g": "GOTO_ENABLED", "p": "ON_POINT_ENABLED", "d": "DOGEAR_ENABLED"}


class FakeEnv(dict):
    """The parts of a PlatformIO SCons env that git_branch.py uses."""

    def __init__(self, package=None, pioenv="someenv", build_flags=(), src_filter=None):
        super().__init__(PIOENV=pioenv, PROJECT_DIR=str(ROOT), BUILD_FLAGS=list(build_flags))
        if src_filter is not None:
            self["SRC_FILTER"] = src_filter
        self.package = package
        self.defines = []

    def GetProjectOption(self, name, default):
        assert name == "custom_package"
        return default if self.package is None else self.package

    def Append(self, CPPDEFINES=()):
        self.defines.extend(CPPDEFINES)

    def Replace(self, SRC_FILTER=None):
        self["SRC_FILTER"] = SRC_FILTER

    def app_flags(self):
        return {name for name, value in self.defines if name.endswith("_ENABLED") and value == 1}

    def define(self, name):
        values = [v for n, v in self.defines if n == name]
        return values[0].strip('\\"') if values else None


def build(package=None, **kw):
    env = FakeEnv(package, **kw)
    with contextlib.redirect_stdout(io.StringIO()):
        gb.inject_version(env)
    return env


class PackageLetters(unittest.TestCase):
    def assertApps(self, env, letters):
        self.assertEqual(env.app_flags(), {FLAGS[x] for x in letters})
        excluded = {f"-<{APP_DIRS[x]}/>" for x in APP_DIRS if x not in letters}
        self.assertEqual(set(env.get("SRC_FILTER", ["+<*>"])) - {"+<*>"}, excluded)

    def test_no_package_keeps_goto_and_on_point_but_never_dogear(self):
        for package in (None, "", "  ", "\t"):
            with self.subTest(package=package):
                env = build(package)
                self.assertApps(env, "gp")
                self.assertNotIn("DOGEAR_ENABLED", env.app_flags())
                self.assertIn("-<activities/dogear/>", env["SRC_FILTER"])
                self.assertIsNone(env.define("CROSSPOINT_VERSION"))  # no suite identity
                self.assertIsNone(env.define("CROSSPOINT_PACKAGE"))

    def test_no_package_with_wolfssl_flags_still_gets_no_dogear(self):
        env = build("", build_flags=WOLF)
        self.assertApps(env, "gp")

    def test_identity_only_env_has_a_version_but_no_package(self):
        env = build("", pioenv="sticky")
        self.assertApps(env, "gp")
        self.assertRegex(env.define("CROSSPOINT_VERSION"), r"^\d+\.\d+\.\d+-A\d+\.\d+\.\d+$")
        self.assertIsNone(env.define("CROSSPOINT_PACKAGE"))

    def test_g_and_p_keep_their_meaning(self):
        for package, letters, suffix in (("g", "g", "G"), ("p", "p", "P"), ("gp", "gp", "GP"), ("pg", "gp", "GP"),
                                         ("GP", "gp", "GP"), (" gp ", "gp", "GP")):
            with self.subTest(package=package):
                env = build(package)
                self.assertApps(env, letters)
                self.assertTrue(env.define("CROSSPOINT_VERSION").endswith("-" + suffix))
                self.assertEqual(env.define("CROSSPOINT_PACKAGE"), letters)

    def test_d_combinations_with_the_wolfssl_flags(self):
        for package, letters in (("d", "d"), ("gd", "gd"), ("dg", "gd"), ("pd", "pd"), ("dp", "pd"),
                                 ("gpd", "gpd"), ("dpg", "gpd"), ("pgd", "gpd"), ("GPD", "gpd")):
            with self.subTest(package=package):
                env = build(package, build_flags=WOLF)
                self.assertApps(env, letters)
                self.assertEqual(env.define("CROSSPOINT_PACKAGE"), letters)
                self.assertTrue(env.define("CROSSPOINT_VERSION").endswith("-" + letters.upper()))

    def test_d_without_the_wolfssl_flags_is_refused(self):
        for package in ("d", "gd", "pd", "gpd"):
            for flags in ((), WOLF[:1], WOLF[1:]):
                with self.subTest(package=package, flags=flags), self.assertRaises(ValueError) as ctx:
                    build(package, build_flags=flags)
                missing = [f for f in WOLF if f not in flags]
                for f in missing:
                    self.assertIn(f, str(ctx.exception))

    def test_invalid_and_duplicate_letters_are_refused(self):
        for package in ("x", "gx", "g,p", "g p", "gg", "gdd", "dd", "ggp", "pp", "gpdg", "a", "gpd!"):
            with self.subTest(package=package), self.assertRaises(ValueError):
                build(package, build_flags=WOLF)

    def test_an_existing_source_filter_is_kept(self):
        env = build("g", src_filter=["+<*>", "-<foo/>"])
        self.assertEqual(env["SRC_FILTER"][:2], ["+<*>", "-<foo/>"])
        self.assertEqual(set(env["SRC_FILTER"][2:]), {"-<activities/on_point/>", "-<activities/dogear/>"})
        env = build("g", src_filter="+<*> -<bar/>")
        self.assertEqual(env["SRC_FILTER"][0], "+<*> -<bar/>")


# ---- platformio.ini: which envs carry DOGEAR --------------------------------

DOGEAR_ENVS = {"default", "x4pro", "x4-goto-dogear-gh_release", "x4pro-goto-dogear-gh_release",
               "x4pro-onpoint-dogear-gh_release", "x4pro-all-gh_release"}


class Ini:
    """Just enough of PlatformIO's ini semantics: `extends` and ${section.option}."""

    def __init__(self, path):
        self.cp = configparser.ConfigParser(interpolation=None, inline_comment_prefixes=(";",), strict=False)
        self.cp.read(path, encoding="utf-8")

    def raw(self, section, option, seen=()):
        if section in seen:
            raise ValueError(f"extends cycle at {section}")
        if self.cp.has_option(section, option):
            return self.cp.get(section, option)
        if section != "env" and self.cp.has_option(section, "extends"):
            for parent in (p.strip() for p in self.cp.get(section, "extends").split(",")):
                value = self.raw(parent, option, seen + (section,))
                if value is not None:
                    return value
        return None

    def get(self, section, option):
        value = self.raw(section, option)
        if value is None:
            return None
        return re.sub(r"\$\{([^.}]+)\.([^}]+)\}", lambda m: self.get(m.group(1), m.group(2)) or "", value)

    def envs(self):
        return [s[len("env:"):] for s in self.cp.sections() if s.startswith("env:")]

    def package(self, env):
        return (self.get(f"env:{env}", "custom_package") or "").strip().lower()

    def flags(self, env):
        return (self.get(f"env:{env}", "build_flags") or "").split()


class IniEnvs(unittest.TestCase):
    ini = Ini(ROOT / "platformio.ini")

    def test_exactly_the_planned_envs_carry_d(self):
        with_d = {e for e in self.ini.envs() if "d" in self.ini.package(e)}
        self.assertEqual(with_d, DOGEAR_ENVS)

    def test_no_env_is_dogear_only(self):
        for env in self.ini.envs():
            self.assertNotEqual(self.ini.package(env), "d", env)

    def test_every_d_env_has_both_wolfssl_flags(self):
        for env in DOGEAR_ENVS:
            for flag in WOLF:
                self.assertIn(flag, self.ini.flags(env), env)

    def test_envs_without_d_do_not_have_them(self):
        for env in self.ini.envs():
            if "d" not in self.ini.package(env):
                for flag in WOLF:
                    self.assertNotIn(flag, self.ini.flags(env), env)

    def test_no_env_defines_dogear_directly(self):
        for env in self.ini.envs():
            self.assertFalse(any("DOGEAR_ENABLED" in f for f in self.ini.flags(env)), env)

    def test_twins_differ_from_their_base_only_by_d_and_the_dogear_flags(self):
        twins = {"x4-goto-dogear-gh_release": "gh_release", "x4pro-goto-dogear-gh_release": "x4pro-goto-gh_release",
                 "x4pro-onpoint-dogear-gh_release": "x4pro-onpoint-gh_release",
                 "x4pro-all-gh_release": "x4pro-gh_release"}
        for twin, base in twins.items():
            with self.subTest(twin=twin):
                self.assertEqual(self.ini.package(twin), "".join(sorted(self.ini.package(base) + "d",
                                                                        key="gpd".index)))
                self.assertEqual(self.ini.flags(twin), self.ini.flags(base) + list(WOLF))
                for option in ("board", "board_build.partitions", "board_build.arduino.memory_type"):
                    self.assertEqual(self.ini.get(f"env:{twin}", option), self.ini.get(f"env:{base}", option))


if __name__ == "__main__":
    unittest.main()
