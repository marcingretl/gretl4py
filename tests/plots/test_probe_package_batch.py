"""Offline tests of native-error detection and stop-on-error batch semantics."""
from __future__ import annotations

import argparse
import csv
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from probe_package_sample import native_error_reason, supervise_probe
from probe_package_batch import (
    CommandResult, bin_database_reference, candidates_from_csv, classify,
    installed_sample_path, run_batch,
)


class NativeErrorTests(unittest.TestCase):
    def test_spawn_status_from_actual_pairplot(self):
        native_log = (
            "gretl_spawn: status = 512: 'error:17: unsupported offsets units: 0.02\n"
            "error:19: unsupported tic modifier: font ',6'\n'\n"
            "Error in call_gridplot: \n[RESULT] Active Matplotlib figures: 0\n"
        )
        self.assertIn("gretl_spawn", native_error_reason(native_log))
        self.assertEqual(classify(CommandResult(0, native_log, native_error_reason(native_log)))[0], "FAIL")

    def test_error_without_spawn_status(self):
        self.assertIn("gnuplot parser", native_error_reason("error:20: unsupported gnuplot command: set pointsize 1\n"))
        self.assertIn("libgretl", native_error_reason("Error in call_gridplot: \n"))
        self.assertIn("failed command", native_error_reason("Failed command: 'gp-executor'\n"))

    def test_non_error_and_no_figure(self):
        self.assertIsNone(native_error_reason("[RESULT] Active Matplotlib figures: 0\n"))
        status, _ = classify(CommandResult(0, "[RESULT] Active Matplotlib figures: 0\n", None))
        self.assertEqual(status, "NO_FIGURES")

    def test_one_figure(self):
        status, _ = classify(CommandResult(0, "[RESULT] Active Matplotlib figures: 1\n", None))
        self.assertEqual(status, "PASS")

    def test_missing_result_marker_not_passed(self):
        status, _ = classify(CommandResult(0, "nothing\n", None))
        self.assertEqual(status, "FAIL")

    def test_supervisor_does_not_silently_pass_native_failure(self):
        from subprocess import CompletedProcess
        result = CompletedProcess([], 0, "gretl_spawn: status = 512: 'error:17: unsupported offsets units: 0.02'\n")
        with patch("probe_package_sample.subprocess.run", return_value=result):
            self.assertEqual(supervise_probe(["PairPlot", "--approved"]), 1)


class BatchTests(unittest.TestCase):
    def args(self, out: Path, *, continue_on_error: bool = False):
        return argparse.Namespace(
            packages=["PairPlot"], candidates=False, catalog=out / "catalog.csv",
            contributed_only=False, offset=0, limit=None, out=out, timeout=60,
            source="auto", samples=None, continue_on_error=continue_on_error,
            skip_bin=False,
        )

    @staticmethod
    def read_report(out: Path):
        with (out / "batch_results.csv").open(newline="", encoding="utf-8") as fh:
            return list(csv.DictReader(fh))

    def test_offset_and_limit_select_next_candidate_batch(self):
        # Offsetting is applied after collecting candidates, not separately to
        # the positional and catalog lists, and before limiting the batch.
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            args = self.args(out)
            args.packages = []
            args.candidates = True
            args.offset = 2
            args.limit = 2
            responses = [
                CommandResult(0, "[EXAMPLE] example.inp\n", None),
                CommandResult(0, "[RESULT] Active Matplotlib figures: 1\n", None),
                CommandResult(0, "[EXAMPLE] example.inp\n", None),
                CommandResult(0, "[RESULT] Active Matplotlib figures: 1\n", None),
            ]
            with patch("probe_package_batch.candidates_from_csv",
                       return_value=["A", "B", "C", "D", "E"]) as candidates:
                with patch("probe_package_batch.run_command", side_effect=responses) as run:
                    self.assertEqual(run_batch(args), 0)
            candidates.assert_called_once_with(args.catalog, False)
            self.assertEqual(run.call_count, 4)
            self.assertEqual([r["package"] for r in self.read_report(out)], ["C", "D"])

    def test_offset_applies_to_explicit_package_list(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            args = self.args(out)
            args.packages = ["A", "B", "C"]
            args.offset = 1
            args.limit = 1
            responses = [
                CommandResult(0, "[EXAMPLE] example.inp\n", None),
                CommandResult(0, "[RESULT] Active Matplotlib figures: 1\n", None),
            ]
            with patch("probe_package_batch.run_command", side_effect=responses):
                self.assertEqual(run_batch(args), 0)
            self.assertEqual(self.read_report(out)[0]["package"], "B")

    def test_offset_past_end_yields_no_selected_packages(self):
        with tempfile.TemporaryDirectory() as tmp:
            args = self.args(Path(tmp))
            args.offset = 5
            with self.assertRaisesRegex(ValueError, "No packages selected"):
                run_batch(args)

    def test_stops_after_first_failure(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            responses = [
                CommandResult(0, "[EXAMPLE] iris1.inp\n[EXAMPLE] growth.inp\n", None),
                CommandResult(0, "[RESULT] Active Matplotlib figures: 0\n", "gretl_spawn: status = 512"),
            ]
            with patch("probe_package_batch.run_command", side_effect=responses) as run:
                self.assertEqual(run_batch(self.args(out)), 1)
                self.assertEqual(run.call_count, 2)
            report = self.read_report(out)
            self.assertEqual(len(report), 1)
            self.assertEqual(report[0]["status"], "FAIL")
            self.assertEqual(report[0]["sample"], "iris1.inp")
            self.assertTrue(Path(report[0]["log"]).is_file())

    def test_continue_on_error(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            responses = [
                CommandResult(0, "[EXAMPLE] iris1.inp\n[EXAMPLE] growth.inp\n", None),
                CommandResult(0, "[RESULT] Active Matplotlib figures: 0\n", "failed"),
                CommandResult(0, "[RESULT] Active Matplotlib figures: 3\n", None),
            ]
            with patch("probe_package_batch.run_command", side_effect=responses) as run:
                self.assertEqual(run_batch(self.args(out, continue_on_error=True)), 1)
                self.assertEqual(run.call_count, 3)
            report = self.read_report(out)
            self.assertEqual([r["status"] for r in report], ["FAIL", "PASS"])

    def test_explicit_samples_filter(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            args = self.args(out)
            args.samples = ["growth"]
            responses = [
                CommandResult(0, "[EXAMPLE] iris1.inp\n[EXAMPLE] growth.inp\n", None),
                CommandResult(0, "[RESULT] Active Matplotlib figures: 1\n", None),
            ]
            with patch("probe_package_batch.run_command", side_effect=responses) as run:
                self.assertEqual(run_batch(args), 0)
                self.assertEqual(run.call_count, 2)
            self.assertEqual(self.read_report(out)[0]["sample"], "growth.inp")

    def test_addon_recursive_examples_run_without_default_fallback(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            args = self.args(out)
            args.packages = ["SVAR"]
            args.source = "addon"
            responses = [
                CommandResult(0, "[EXAMPLE] SetRestrictions/spaghetti_plot.inp\n"
                                 "[EXAMPLE] Traditional/BlQuah.inp\n", None),
                CommandResult(0, "[RESULT] Active Matplotlib figures: 1\n", None),
                CommandResult(0, "[RESULT] Active Matplotlib figures: 1\n", None),
            ]
            with patch("probe_package_batch.run_command", side_effect=responses) as run:
                self.assertEqual(run_batch(args), 0)
                self.assertEqual(run.call_count, 3)
                commands = [call.args[0] for call in run.call_args_list]
            self.assertIn("SetRestrictions/spaghetti_plot.inp", commands[1])
            self.assertIn("Traditional/BlQuah.inp", commands[2])
            self.assertEqual([r["sample"] for r in self.read_report(out)],
                             ["SetRestrictions/spaghetti_plot.inp", "Traditional/BlQuah.inp"])

    def test_selected_nested_example_by_short_name(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            args = self.args(out)
            args.packages = ["SVAR"]
            args.samples = ["BlQuah"]
            responses = [
                CommandResult(0, "[EXAMPLE] SetRestrictions/other.inp\n"
                                 "[EXAMPLE] Traditional/BlQuah.inp\n", None),
                CommandResult(0, "[RESULT] Active Matplotlib figures: 1\n", None),
            ]
            with patch("probe_package_batch.run_command", side_effect=responses) as run:
                self.assertEqual(run_batch(args), 0)
                self.assertEqual(run.call_count, 2)
            self.assertEqual(self.read_report(out)[0]["sample"], "Traditional/BlQuah.inp")

    def test_addon_without_samples_is_no_samples(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            args = self.args(out)
            args.packages = ["SVAR"]
            args.source = "addon"
            with patch("probe_package_batch.run_command", return_value=CommandResult(0, "[EXAMPLES] none\n", None)) as run:
                self.assertEqual(run_batch(args), 0)
                self.assertEqual(run.call_count, 1)
            self.assertEqual(self.read_report(out)[0]["status"], "NO_SAMPLES")

    def test_skip_bin_detector_real_cffilt_example(self):
        source = """# open ignored.bin
# Replicate Christiano and Fitzgerald (2003)
clear
include cffilt.gfn
open fedstl.bin
data m2ns cpiaucns
"""
        with tempfile.TemporaryDirectory() as tmp:
            sample = Path(tmp) / "replication.inp"
            sample.write_text(source, encoding="utf-8")
            self.assertEqual(bin_database_reference(sample), (5, "fedstl.bin"))

    def test_skip_bin_detector_quoted_names_and_comments(self):
        source = """# open ignored.bin
  open 'data/fedstl.bin' --quiet
open "some_other.bin"
"""
        with tempfile.TemporaryDirectory() as tmp:
            sample = Path(tmp) / "sample.inp"
            sample.write_text(source, encoding="utf-8")
            self.assertEqual(bin_database_reference(sample), (2, "data/fedstl.bin"))
            sample.write_text("# open ignored.bin\nopen sample.gdt\n", encoding="utf-8")
            self.assertIsNone(bin_database_reference(sample))

    def test_skip_bin_preflight_avoids_running_example(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            examples = out / "examples"
            examples.mkdir()
            (examples / "replication.inp").write_text(
                "# note\nopen fedstl.bin\n", encoding="utf-8"
            )
            (examples / "artificial.inp").write_text(
                "open generated.gdt\n", encoding="utf-8"
            )
            args = self.args(out)
            args.packages = ["cffilt"]
            args.skip_bin = True
            responses = [
                CommandResult(0, f"[EXAMPLES] {examples}\n[EXAMPLE] replication.inp\n"
                                 f"[EXAMPLE] artificial.inp\n", None),
                CommandResult(0, "[RESULT] Active Matplotlib figures: 1\n", None),
            ]
            with patch("probe_package_batch.run_command", side_effect=responses) as run:
                self.assertEqual(run_batch(args), 0)
                self.assertEqual(run.call_count, 2)
            rows = self.read_report(out)
            self.assertEqual([r["status"] for r in rows], ["SKIPPED_BIN", "PASS"])
            self.assertIn("fedstl.bin", rows[0]["reason"])
            self.assertIn("SKIPPED_BIN", Path(rows[0]["log"]).read_text())

    def test_skip_bin_does_not_affect_default_behavior(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            args = self.args(out)
            responses = [
                CommandResult(0, "[EXAMPLE] replication.inp\n", None),
                CommandResult(1, "[FAIL] FileNotFoundError: fedstl.bin\n", "failed"),
            ]
            with patch("probe_package_batch.run_command", side_effect=responses) as run:
                self.assertEqual(run_batch(args), 1)
                self.assertEqual(run.call_count, 2)
            self.assertEqual(self.read_report(out)[0]["status"], "FAIL")

    def test_installed_sample_path_rejects_escape(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.assertEqual(installed_sample_path(f"[EXAMPLES] {root}\n", "a.inp"), root / "a.inp")
            with self.assertRaises(ValueError):
                installed_sample_path(f"[EXAMPLES] {root}\n", "../bad.inp")

    def test_catalog_excludes_no_plot(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            csvfile = out / "plot_packages.csv"
            csvfile.write_text("package,origin,status\nPairPlot,contributed,plot_detected\nNoGraphs,contributed,no_plot_detected\n", encoding="utf-8")
            self.assertEqual(candidates_from_csv(csvfile, False), ["PairPlot"])


class CustomSuiteBatchTests(unittest.TestCase):
    """Custom plotting fixtures use the existing batch runner and CSV format."""

    def arguments(self, out: Path, **kwargs):
        args = argparse.Namespace(
            packages=[], candidates=False, catalog=out / "catalog.csv",
            contributed_only=False, offset=0, limit=None, out=out, timeout=60,
            source="auto", samples=None, continue_on_error=False,
            skip_bin=False, custom="priority5", cases=None,
        )
        for name, value in kwargs.items():
            setattr(args, name, value)
        return args

    def test_custom_cases_use_existing_batch_protocol(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            args = self.arguments(out, cases=["02_color_xy", "04_candlesticks_basic"])
            responses = [
                CommandResult(0, "[RESULT] Active Matplotlib figures: 2\n", None),
                CommandResult(0, "[RESULT] Active Matplotlib figures: 0\n", None),
            ]
            with patch("probe_package_batch.run_command", side_effect=responses) as run:
                self.assertEqual(run_batch(args), 0)
            commands = [call.args[0] for call in run.call_args_list]
            self.assertEqual(len(commands), 2)
            self.assertIn("--custom", commands[0])
            self.assertIn("priority5", commands[0])
            self.assertIn("02_color_xy", commands[0])
            self.assertIn("04_candlesticks_basic", commands[1])
            with (out / "batch_results.csv").open(newline="", encoding="utf-8") as fh:
                rows = list(csv.DictReader(fh))
            self.assertEqual([r["status"] for r in rows], ["PASS", "NO_FIGURES"])
            self.assertEqual([r["package"] for r in rows], ["color_xy", "candlesticks"])
            self.assertEqual(rows[0]["sample"], "custom/priority5/02_color_xy.inp")
            self.assertTrue(all(Path(r["log"]).is_file() for r in rows))

    def test_custom_failure_stops_by_default(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            args = self.arguments(out, cases=["01_TVCplots", "02_color_xy"])
            with patch("probe_package_batch.run_command", return_value=CommandResult(
                    1, "[FAIL] unexpected error\n", "process exited with status 1")) as run:
                self.assertEqual(run_batch(args), 1)
                self.assertEqual(run.call_count, 1)
            with (out / "batch_results.csv").open(newline="", encoding="utf-8") as fh:
                rows = list(csv.DictReader(fh))
            self.assertEqual([r["status"] for r in rows], ["FAIL"])

    def test_custom_continue_and_limit(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            args = self.arguments(out, cases=["01_TVCplots", "02_color_xy", "06_SankeyPlot"],
                                  offset=1, limit=2, continue_on_error=True)
            responses = [
                CommandResult(1, "[FAIL] error\n", "process exited with status 1"),
                CommandResult(0, "[RESULT] Active Matplotlib figures: 1\n", None),
            ]
            with patch("probe_package_batch.run_command", side_effect=responses) as run:
                self.assertEqual(run_batch(args), 1)
            self.assertEqual(run.call_count, 2)
            with (out / "batch_results.csv").open(newline="", encoding="utf-8") as fh:
                rows = list(csv.DictReader(fh))
            self.assertEqual([r["status"] for r in rows], ["FAIL", "PASS"])
            self.assertIn("02_color_xy", rows[0]["sample"])
            self.assertIn("06_SankeyPlot", rows[1]["sample"])

    def test_custom_unknown_case_rejected_before_execution(self):
        with tempfile.TemporaryDirectory() as tmp:
            args = self.arguments(Path(tmp), cases=["not_a_case"])
            with patch("probe_package_batch.run_command") as run:
                with self.assertRaisesRegex(ValueError, "Unknown cases"):
                    run_batch(args)
                run.assert_not_called()

    def test_custom_bin_preflight_uses_existing_skip_logic(self):
        from probe_package_sample import CustomCase
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            script = out / "samples" / "bin_test.inp"
            script.parent.mkdir()
            script.write_text("# test\nopen foo.bin\n", encoding="utf-8")
            args = self.arguments(out, skip_bin=True)
            with patch("probe_package_batch.load_custom_cases",
                       return_value=[CustomCase("bin_test", "BACE", script, out)]):
                with patch("probe_package_batch.run_command") as run:
                    self.assertEqual(run_batch(args), 0)
                    run.assert_not_called()
            with (out / "batch_results.csv").open(newline="", encoding="utf-8") as fh:
                rows = list(csv.DictReader(fh))
            self.assertEqual(rows[0]["status"], "SKIPPED_BIN")


if __name__ == "__main__":
    unittest.main()
