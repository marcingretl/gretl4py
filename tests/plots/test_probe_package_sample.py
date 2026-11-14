"""Offline tests of package lookup and conditional local installation."""
from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from probe_package_sample import (
    find_downloaded_contributed, load_package,
    choose_sample_script, list_sample_scripts, installed_examples_dir, resolve_sample_name,
    sample_working_directory,
)


class FakeGretl:
    def __init__(self, installed=()):
        self.installed = set(installed)
        self.includes = []
        self.installs = []

    def include(self, name):
        self.includes.append(name)
        if name not in self.installed:
            raise FileNotFoundError(name)
        return f"loaded:{name}"

    def pkg(self, command, path, *, local):
        self.installs.append((command, path, local))
        self.installed.add(Path(path).stem)


class ProbeTests(unittest.TestCase):
    def test_addon_never_installs_even_if_cached(self):
        with tempfile.TemporaryDirectory() as temp:
            p = Path(temp)
            (p / "SVAR.gfn").write_bytes(b"example")
            api = FakeGretl(["SVAR"])
            self.assertEqual(load_package(api, "SVAR", "addon", p), "loaded:SVAR")
            self.assertEqual(api.installs, [])
            self.assertEqual(api.includes, ["SVAR"])

    def test_preinstalled_contributed_skips_install(self):
        with tempfile.TemporaryDirectory() as temp:
            p = Path(temp)
            (p / "PairPlot.zip").write_bytes(b"example")
            api = FakeGretl(["PairPlot"])
            self.assertEqual(load_package(api, "PairPlot", "auto", p), "loaded:PairPlot")
            self.assertEqual(api.installs, [])
            self.assertEqual(api.includes, ["PairPlot"])

    def test_missing_contributed_uses_cached_zip(self):
        with tempfile.TemporaryDirectory() as temp:
            p = Path(temp)
            archive = p / "PairPlot.zip"
            archive.write_bytes(b"example")
            api = FakeGretl()
            self.assertEqual(load_package(api, "PairPlot", "auto", p), "loaded:PairPlot")
            self.assertEqual(api.includes, ["PairPlot", "PairPlot"])
            self.assertEqual(api.installs, [("install", str(archive.resolve()), True)])

    def test_gfn_preferred_over_zip(self):
        with tempfile.TemporaryDirectory() as temp:
            p = Path(temp)
            (p / "BACE.zip").write_bytes(b"example")
            gfn = p / "BACE.gfn"
            gfn.write_bytes(b"example")
            self.assertEqual(find_downloaded_contributed("BACE", p), gfn.resolve())

    def test_missing_explicit_contributed_is_reported_without_download(self):
        with tempfile.TemporaryDirectory() as temp:
            api = FakeGretl()
            with self.assertRaisesRegex(FileNotFoundError, "no cached"):
                load_package(api, "Missing", "contributed", Path(temp))
            self.assertEqual(api.installs, [])

    def test_unknown_uncached_auto_only_includes(self):
        with tempfile.TemporaryDirectory() as temp:
            api = FakeGretl(["SVAR"])
            self.assertEqual(load_package(api, "SVAR", "auto", Path(temp)), "loaded:SVAR")
            self.assertEqual(api.installs, [])

    def test_case_insensitive_archive_lookup(self):
        with tempfile.TemporaryDirectory() as temp:
            p = Path(temp)
            gfn = p / "tvcplots.GFN"
            gfn.write_bytes(b"example")
            self.assertEqual(find_downloaded_contributed("TVCplots", p), gfn.resolve())

    def test_reject_path_as_package_name(self):
        with self.assertRaises(ValueError):
            find_downloaded_contributed("../Package")

    def test_named_pairplot_samples_require_explicit_selection(self):
        with tempfile.TemporaryDirectory() as temp:
            examples = Path(temp) / "PairPlot" / "examples"
            examples.mkdir(parents=True)
            for name in ("credscore.inp", "growth.inp", "iris1.inp", "iris2.inp", "panel.inp", "school.inp"):
                (examples / name).write_text("# example\n")
            self.assertEqual(len(list_sample_scripts(examples)), 6)
            with self.assertRaisesRegex(ValueError, "multiple examples"):
                choose_sample_script(examples)
            self.assertEqual(choose_sample_script(examples, "iris1"), (examples / "iris1.inp").resolve())
            self.assertEqual(choose_sample_script(examples, "iris1.inp"), (examples / "iris1.inp").resolve())

    def test_conventional_sample_is_preferred(self):
        with tempfile.TemporaryDirectory() as temp:
            examples = Path(temp) / "SVAR" / "examples"
            examples.mkdir(parents=True)
            (examples / "SVAR_sample.inp").write_text("# canonical\n")
            (examples / "other.inp").write_text("# extra\n")
            self.assertEqual(choose_sample_script(examples), (examples / "SVAR_sample.inp").resolve())

    def test_single_named_sample_is_selected(self):
        with tempfile.TemporaryDirectory() as temp:
            examples = Path(temp) / "Custom" / "examples"
            examples.mkdir(parents=True)
            (examples / "show.inp").write_text("# sample\n")
            self.assertEqual(choose_sample_script(examples), (examples / "show.inp").resolve())

    def test_missing_and_unsafe_sample_names(self):
        with tempfile.TemporaryDirectory() as temp:
            examples = Path(temp) / "Sample" / "examples"
            examples.mkdir(parents=True)
            (examples / "sample.inp").write_text("# sample\n")
            with self.assertRaises(FileNotFoundError):
                choose_sample_script(examples, "missing")
            with self.assertRaises(ValueError):
                choose_sample_script(examples, "../escape.inp")
            with self.assertRaises(ValueError):
                choose_sample_script(examples, "/etc/passwd")

    def test_sample_symlink_outside_directory_is_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            examples = root / "Foo" / "examples"
            examples.mkdir(parents=True)
            outside = root / "outside.inp"
            outside.write_text("# outside\n")
            (examples / "escape.inp").symlink_to(outside)
            with self.assertRaisesRegex(ValueError, "outside"):
                choose_sample_script(examples, "escape")

    def test_recursive_svar_examples_keep_relative_paths(self):
        with tempfile.TemporaryDirectory() as temp:
            examples = Path(temp) / "SVAR" / "examples"
            (examples / "Traditional").mkdir(parents=True)
            (examples / "SetRestrictions").mkdir(parents=True)
            for name in ("Traditional/BlQuah.inp", "Traditional/plain.inp",
                         "SetRestrictions/spaghetti_plot.inp"):
                (examples / name).write_text("# sample\n")
            found = list_sample_scripts(examples)
            names = [p.relative_to(examples).as_posix() for p in found]
            self.assertEqual(names, [
                "SetRestrictions/spaghetti_plot.inp", "Traditional/BlQuah.inp",
                "Traditional/plain.inp",
            ])
            self.assertEqual(choose_sample_script(examples, "Traditional/BlQuah"),
                             (examples / "Traditional/BlQuah.inp").resolve())
            self.assertEqual(choose_sample_script(examples, "BlQuah"),
                             (examples / "Traditional/BlQuah.inp").resolve())
            with self.assertRaisesRegex(ValueError, "multiple examples"):
                choose_sample_script(examples)

    def test_duplicate_basename_requires_relative_path(self):
        with tempfile.TemporaryDirectory() as temp:
            examples = Path(temp) / "SVAR" / "examples"
            for folder in ("A", "B"):
                target = examples / folder
                target.mkdir(parents=True)
                (target / "test.inp").write_text("# sample\n")
            available = ["A/test.inp", "B/test.inp"]
            with self.assertRaisesRegex(ValueError, "Ambiguous sample"):
                resolve_sample_name(available, "test")
            self.assertEqual(resolve_sample_name(available, "B/test"), "B/test.inp")
            with self.assertRaisesRegex(ValueError, "Ambiguous sample"):
                choose_sample_script(examples, "test")

    def test_nested_traversal_and_symlink_are_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            examples = root / "SVAR" / "examples"
            nested = examples / "Traditional"
            nested.mkdir(parents=True)
            external = root / "outside.inp"
            external.write_text("# external\n")
            (nested / "external.inp").symlink_to(external)
            for name in ("../outside.inp", "Traditional/../other.inp", "/tmp/foo.inp",
                         "Traditional//test.inp", "C:/foo.inp", r"Traditional\foo.inp"):
                with self.subTest(name=name):
                    with self.assertRaises(ValueError):
                        resolve_sample_name(["Traditional/external.inp"], name)
            with self.assertRaisesRegex(ValueError, "outside"):
                choose_sample_script(examples, "Traditional/external.inp")

    def test_package_relative_bvar_data_changes_only_affected_sample(self):
        with tempfile.TemporaryDirectory() as temp:
            package = Path(temp) / "BVAR"
            examples = package / "examples"
            nested = examples / "Usage_examples"
            nested.mkdir(parents=True)
            data = nested / "CONSUMPTIONLOGALL.gdt"
            data.write_text("data")
            script = nested / "BVAR_large.inp"
            script.write_text(
                "# BVAR - large BVAR example\n"
                "include BVAR.gfn\n"
                "open examples/Usage_examples/CONSUMPTIONLOGALL.gdt\n"
            )
            self.assertEqual(sample_working_directory(script, examples), package.resolve())

            # Existing examples without package-root paths keep their cwd.
            other = nested / "regular.inp"
            other.write_text("open data.gdt\n")
            self.assertIsNone(sample_working_directory(other, examples))

    def test_package_relative_open_must_exist_and_stay_inside_package(self):
        with tempfile.TemporaryDirectory() as temp:
            package = Path(temp) / "BVAR"
            examples = package / "examples"
            examples.mkdir(parents=True)
            script = examples / "check.inp"
            script.write_text("# open examples/foo.gdt\nopen examples/missing.gdt\n")
            self.assertIsNone(sample_working_directory(script, examples))

            data = examples / "quoted data.gdt"
            data.write_text("data")
            script.write_text('open "examples/quoted data.gdt"\n')
            self.assertEqual(sample_working_directory(script, examples), package.resolve())

    def test_installed_examples_dir_uses_gretl_module_path(self):
        class FakeModule:
            __file__ = "/tmp/testsite/gretl/__init__.py"
        expected = Path("/tmp/testsite/gretl/functions/PairPlot/examples")
        self.assertEqual(installed_examples_dir(FakeModule, "PairPlot"), expected)


class CustomSuiteTests(unittest.TestCase):
    """Reviewed fixtures cannot escape the audit tree and sync gretl's workdir."""

    def test_priority5_manifest_has_twelve_valid_scripts(self):
        from probe_package_sample import load_custom_cases
        cases = load_custom_cases("priority5")
        # Keep the six passing original cases and append six more reviewed cases.
        self.assertEqual(len(cases), 12)
        self.assertEqual(
            [c.name for c in cases[:6]],
            ["01_TVCplots", "02_color_xy", "03_gnuplot_piechart",
             "04_candlesticks_basic", "05_candlesticks_volume", "06_SankeyPlot"],
        )
        self.assertEqual(
            [c.name for c in cases[6:]],
            ["07_bandplot", "08_kddensity_contour", "09_kddensity_heatmap",
             "10_season_plot", "11_MDS", "12_corresp"],
        )
        self.assertEqual([c.package for c in cases].count("candlesticks"), 2)
        self.assertTrue(all(c.script.is_file() for c in cases))
        self.assertTrue(all(c.script.is_relative_to(c.workdir) for c in cases))

    def test_reject_unsafe_suite_and_manifest_paths(self):
        from probe_package_sample import load_custom_cases
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.assertRaises(ValueError, load_custom_cases, "../bad", root)
            folder = root / "bad_suite"
            folder.mkdir()
            (folder / "manifest.csv").write_text(
                "case,package,script\nevil,Evil,samples/../../outside.inp\n", encoding="utf-8"
            )
            with self.assertRaisesRegex(ValueError, "Invalid custom fixture path"):
                load_custom_cases("bad_suite", root)

    def test_reject_symlink_escape_and_duplicate_case(self):
        from probe_package_sample import load_custom_cases
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            folder = root / "suite"
            (folder / "samples").mkdir(parents=True)
            outside = root / "outside.inp"
            outside.write_text("# external\n", encoding="utf-8")
            (folder / "samples" / "test.inp").symlink_to(outside)
            (folder / "manifest.csv").write_text(
                "case,package,script\ntest,TVCplots,samples/test.inp\n", encoding="utf-8"
            )
            with self.assertRaisesRegex(ValueError, "Missing or unsafe"):
                load_custom_cases("suite", root)
            (folder / "samples" / "test.inp").unlink()
            (folder / "samples" / "test.inp").write_text("# valid\n")
            (folder / "manifest.csv").write_text(
                "case,package,script\ntest,TVCplots,samples/test.inp\n"
                "test,TVCplots,samples/test.inp\n", encoding="utf-8"
            )
            with self.assertRaisesRegex(ValueError, "duplicate"):
                load_custom_cases("suite", root)

    def test_workdir_helper_updates_python_and_libgretl(self):
        from probe_package_sample import execute_with_workdir
        from unittest.mock import Mock
        import os
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp).resolve()
            script = root / "case.inp"
            script.write_text("# test\n")
            start = Path.cwd()
            api = Mock()
            def on_run(_script):
                self.assertEqual(Path.cwd(), root)
            api.run_script.side_effect = on_run
            execute_with_workdir(api, script, root)
            self.assertEqual(Path.cwd(), start)
            api.run_hansl.assert_called_once_with(f'set workdir "{root}"')
            api.run_script.assert_called_once_with(str(script))


if __name__ == "__main__":
    unittest.main()
