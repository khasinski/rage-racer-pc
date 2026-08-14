import json
import tempfile
import unittest
from pathlib import Path

from tools.scripts.progress_report import (
    carries_assembly,
    color,
    count,
    percent,
    source_mix,
    table,
)

REPORT = {
    "measures": {
        "matched_functions": 1190,
        "total_functions": 1197,
        "matched_functions_percent": 99.42,
        "matched_code_percent": 99.87,
        "matched_data_percent": 99.82,
    },
    "categories": [
        {
            "id": "game",
            "name": "game",
            "measures": {
                "matched_functions": 701,
                "total_functions": 701,
                "matched_functions_percent": 100.0,
                "matched_code_percent": 100.0,
                "matched_data_percent": 99.88,
            },
        },
        {
            "id": "psyq",
            "name": "psyq",
            # objdiff omits a measure entirely rather than writing a zero.
            "measures": {
                "matched_functions": 489,
                "total_functions": 496,
                "matched_functions_percent": 98.59,
                "matched_code_percent": 99.43,
            },
        },
    ],
}


class MeasureReadingTest(unittest.TestCase):
    def test_a_missing_measure_reads_as_zero_not_as_a_crash(self):
        # objdiff leaves a field out when nothing matched, so `or 0.0` is doing
        # real work here rather than guarding against a hypothetical.
        self.assertEqual(percent({}, "matched_code_percent"), 0.0)
        self.assertEqual(count({}, "matched_functions"), 0)

    def test_an_explicit_null_reads_as_zero(self):
        self.assertEqual(percent({"matched_data_percent": None}, "matched_data_percent"), 0.0)
        self.assertEqual(count({"matched_data": None}, "matched_data"), 0)


class TableTest(unittest.TestCase):
    def test_reports_both_categories_and_the_whole_executable(self):
        rows = "\n".join(table(REPORT))
        self.assertIn("Game code", rows)
        self.assertIn("PsyQ libraries", rows)
        self.assertIn("Whole executable", rows)

    def test_carries_the_real_numbers_through(self):
        rows = "\n".join(table(REPORT))
        self.assertIn("701 / 701 (100.00%)", rows)
        self.assertIn("1190 / 1197 (99.42%)", rows)

    def test_a_category_missing_a_measure_still_renders(self):
        # PsyQ has no data measure at all in the fixture; the row must not
        # vanish or throw, because a dropped row silently shrinks the report.
        rows = [r for r in table(REPORT) if "PsyQ" in r]
        self.assertEqual(len(rows), 1)
        self.assertIn("0.00%", rows[0])


class ColorTest(unittest.TestCase):
    def test_only_a_complete_result_is_bright(self):
        self.assertEqual(color(100.0), "brightgreen")
        self.assertEqual(color(99.9), "green")

    def test_a_bad_result_is_not_dressed_up(self):
        self.assertEqual(color(35.0), "orange")
        self.assertEqual(color(20.0), "red")
        self.assertEqual(color(0.0), "red")


class CarriesAssemblyTest(unittest.TestCase):
    def test_plain_c_is_plain(self):
        self.assertFalse(carries_assembly("void f(void) {\n    return;\n}\n"))

    def test_an_include_counts(self):
        self.assertTrue(carries_assembly('HANDWRITTEN_ASM("a/b", Fn);\n'))

    def test_an_inline_block_counts(self):
        self.assertTrue(carries_assembly('void f(void) {\n    asm volatile("nop");\n}\n'))

    def test_a_directive_only_counts_inside_a_string(self):
        # `foo.word` on a union with a `word` member is not assembly, and
        # reading it as such silently reclassified plain C for a whole release.
        self.assertFalse(carries_assembly("s32 v = state.word + 1;\n"))
        self.assertTrue(carries_assembly('__asm__(".word 0x0\\n");\n'))


class SourceMixTest(unittest.TestCase):
    def test_counts_units_not_functions(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "plain.c").write_text("void a(void) {}\nvoid b(void) {}\n")
            (root / "mixed.c").write_text('void a(void) {}\nHANDWRITTEN_ASM("x", B);\n')
            self.assertEqual(source_mix(root), (1, 2))

    def test_a_unit_mixing_c_and_assembly_is_not_plain(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "mixed.c").write_text('void a(void) {}\nasm volatile("nop");\n')
            self.assertEqual(source_mix(root), (0, 1))


class ReportShapeTest(unittest.TestCase):
    def test_the_real_report_has_what_the_table_needs(self):
        # Guards against an objdiff upgrade renaming a field: the table would
        # otherwise render zeroes and look like a regression.
        path = Path(__file__).resolve().parents[2] / "build" / "PAL" / "report.json"
        if not path.exists():
            self.skipTest("run `make report` first")
        report = json.loads(path.read_text())
        self.assertIn("measures", report)
        self.assertEqual({c["id"] for c in report.get("categories", [])}, {"game", "psyq"})
        for key in ("matched_functions", "total_functions", "matched_code_percent"):
            self.assertIn(key, report["measures"])


if __name__ == "__main__":
    unittest.main()
