import unittest

from tools.scripts.gen_objdiff_config import (
    CATEGORIES,
    category,
    config,
    linked_objects,
    unit_name,
)

MAP = """
 .text          0x80013f48      0x654 build/PAL/src/main/PAL/main/pad/init_pad.c.o
 .rodata        0x80011404       0x14 build/PAL/src/main/PAL/main/pad/init_pad.c.o
 .text          0x80069c94       0x40 build/PAL/src/main/PAL/lib/libgte/leading_zero_count.c.o
 .data          0x8007b664        0x0 build/PAL/asm/PAL/main/data/main/6BE64.data.s.o
 .text          0x00000000        0x0 build/USA/src/main/USA/main/pad/init_pad.c.o
"""


class LinkedObjectsTest(unittest.TestCase):
    def test_lists_each_object_once_in_link_order(self):
        self.assertEqual(linked_objects(MAP, "PAL"), [
            "src/main/PAL/main/pad/init_pad.c.o",
            "src/main/PAL/lib/libgte/leading_zero_count.c.o",
            "asm/PAL/main/data/main/6BE64.data.s.o",
        ])

    def test_ignores_another_version(self):
        self.assertEqual(linked_objects(MAP, "USA"), ["src/main/USA/main/pad/init_pad.c.o"])


class UnitNameTest(unittest.TestCase):
    def test_drops_the_build_scaffolding(self):
        self.assertEqual(unit_name("src/main/PAL/main/pad/init_pad.c.o"), "PAL/main/pad/init_pad")

    def test_names_an_assembled_blob_too(self):
        self.assertEqual(unit_name("asm/PAL/main/data/main/6BE64.data.s.o"),
                         "PAL/main/data/main/6BE64.data")


class CategoryTest(unittest.TestCase):
    def test_the_psyq_libraries_are_their_own_category(self):
        self.assertEqual(category("src/main/PAL/lib/libgte/leading_zero_count.c.o"), "psyq")
        self.assertEqual(category("src/main/PAL/main/pad/init_pad.c.o"), "game")

    def test_a_data_blob_counts_as_game_code(self):
        # It is the game's data, and it has to land somewhere for the two
        # category totals to add up to the whole executable.
        self.assertEqual(category("asm/PAL/main/data/main/6BE64.data.s.o"), "game")

    def test_every_category_used_is_declared(self):
        # objdiff drops a unit's category silently if the config never declares
        # it, which would quietly shrink a subtotal.
        declared = {c["id"] for c in CATEGORIES}
        for relative in ("src/main/PAL/lib/libgte/lzc.c.o", "src/main/PAL/main/pad/init_pad.c.o"):
            self.assertIn(category(relative), declared)


class ConfigTest(unittest.TestCase):
    def test_target_and_base_mirror_each_other(self):
        written = config(["src/main/PAL/main/pad/init_pad.c.o"], "PAL", "expected")
        unit = written["units"][0]
        self.assertEqual(unit["base_path"], "build/PAL/src/main/PAL/main/pad/init_pad.c.o")
        self.assertEqual(unit["target_path"],
                         "expected/PAL/build/src/main/PAL/main/pad/init_pad.c.o")

    def test_every_unit_carries_its_category(self):
        written = config(["src/main/PAL/lib/libgte/lzc.c.o"], "PAL", "expected")
        self.assertEqual(written["units"][0]["metadata"]["progress_categories"], ["psyq"])

    def test_nothing_is_excluded_from_the_report(self):
        # decomp.dev requires every function in the binary to be accounted for.
        objects = ["src/main/PAL/main/pad/init_pad.c.o", "src/main/PAL/lib/libgte/lzc.c.o"]
        written = config(objects, "PAL", "expected")
        self.assertEqual(len(written["units"]), len(objects))

    def test_units_are_marked_complete(self):
        # The tree links to the original SHA-1, and `make check` proves it
        # before this ever runs.
        written = config(["src/main/PAL/main/pad/init_pad.c.o"], "PAL", "expected")
        self.assertTrue(written["units"][0]["metadata"]["complete"])

    def test_objdiff_is_told_not_to_build_the_target(self):
        # gen_expected.py owns that side; letting objdiff run make over it
        # would rebuild the base on top of it.
        written = config([], "PAL", "expected")
        self.assertFalse(written["build_target"])


if __name__ == "__main__":
    unittest.main()
