import tempfile
import unittest
from pathlib import Path

from tools.scripts.code_debt import count_debt


class CodeDebtTest(unittest.TestCase):
    def test_counts_each_scaffolding_family(self):
        source = r'''
extern s32 misplaced;
#define GAME_SAMPLE_TYPE s16
#define GAME_SAMPLE_DECL extern s32 sample
void f(u8 *base, void *ptr) {
    Address address;
    register s32 value asm("$4");
    register s32 named asm("v0");
    value = *(s16 *)(base + 0x10);
    value += *(s32 *)(base - 8);
    value += (*(u8 *)(&object->member)) - 1;
    value += *(volatile s16 *)&object->other_member;
    value += ((GameObject *)&object->storage)->value;
    ptr = (void *)((u8 *)ptr + 4);
    value += (s32)ptr;
    value += FIELD32(base, 4);
    value += RAW(object->member);
    value += base[-2];
    address.byteOffset += 4;
    sampleAddress.value += 4;
    asm volatile("");
    value += ({ s32 temporary = 2; temporary; });
    asm(".globl func_80001234\nfunc_80001234 = f + 4");
    value += object->field_20 + object->unk14;
}
'''
        with tempfile.TemporaryDirectory() as directory:
            Path(directory, "sample.c").write_text(source)
            counts = count_debt(Path(directory))

        self.assertEqual(counts["byte_pointer_arithmetic"], 1)
        self.assertEqual(counts["raw_offset_dereferences"], 2)
        self.assertEqual(counts["pointer_integer_casts"], 1)
        self.assertEqual(counts["address_reinterpret_casts"], 2)
        self.assertEqual(counts["aggregate_address_casts"], 1)
        self.assertEqual(counts["explicit_pointer_casts"], 7)
        self.assertEqual(counts["manual_byte_offsets"], 1)
        self.assertEqual(counts["address_integer_arithmetic"], 1)
        self.assertEqual(counts["negative_pointer_indexing"], 1)
        self.assertEqual(counts["field_macros"], 2)
        self.assertEqual(counts["register_pins"], 2)
        self.assertEqual(counts["empty_barriers"], 1)
        self.assertEqual(counts["statement_expressions"], 1)
        self.assertEqual(counts["asm_aliases"], 1)
        self.assertEqual(counts["unknown_fields"], 2)
        self.assertEqual(counts["externs_in_c"], 1)
        self.assertEqual(counts["declaration_overrides"], 2)

    def test_ignores_comments(self):
        with tempfile.TemporaryDirectory() as directory:
            Path(directory, "sample.c").write_text(
                "/* extern s32 x; *(s32 *)(p + 0x10); asm(\"alias\"); */\n"
            )
            counts = count_debt(Path(directory))

        self.assertTrue(all(count == 0 for count in counts.values()))

    def test_excludes_pins_required_by_handwritten_asm(self):
        source = r'''
/* HANDWRITTEN_ASM */
void f(void) {
    register s32 value asm("$2");
    asm volatile("move %0, $3" : "=r"(value));
}
'''
        with tempfile.TemporaryDirectory() as directory:
            Path(directory, "sample.c").write_text(source)
            counts = count_debt(Path(directory))

        self.assertEqual(counts["register_pins"], 0)

    def test_byte_pointer_arithmetic_ignores_casts_of_completed_expressions(self):
        source = r'''
void f(u8 *bytes, Packet *packet, u8 value) {
    bytes = (u8 *)bytes + 4;
    bytes = (u8 *)(packet + 1);
    *(volatile u8 *)&packet->tag = value + 1;
}
'''
        with tempfile.TemporaryDirectory() as directory:
            Path(directory, "sample.c").write_text(source)
            counts = count_debt(Path(directory))

        self.assertEqual(counts["byte_pointer_arithmetic"], 1)
        self.assertEqual(counts["explicit_pointer_casts"], 3)

    def test_pointer_integer_casts_ignore_integer_expressions(self):
        source = r'''
void f(void *pointer, s32 value) {
    value = (s32)pointer;
    value += (s32)(pointer + 3);
    value += (u32)((-0x100) - value) << 8;
}
'''
        with tempfile.TemporaryDirectory() as directory:
            Path(directory, "sample.c").write_text(source)
            counts = count_debt(Path(directory))

        self.assertEqual(counts["pointer_integer_casts"], 2)

    def test_counts_asm_aliases_in_game_headers(self):
        with tempfile.TemporaryDirectory() as source_directory, tempfile.TemporaryDirectory() as header_directory:
            Path(source_directory, "sample.c").write_text("void f(void) {}\n")
            Path(header_directory, "sample.h").write_text(
                'extern s32 alias asm("canonical");\n'
                'extern s32 hardware asm("0x1F800000");\n'
                'extern s32 ordinary;\n'
            )
            counts = count_debt(Path(source_directory), Path(header_directory))

        self.assertEqual(counts["header_asm_aliases"], 1)

    def test_counts_address_integer_arithmetic_in_game_headers(self):
        with tempfile.TemporaryDirectory() as source_directory, tempfile.TemporaryDirectory() as header_directory:
            Path(source_directory, "sample.c").write_text("void f(void) {}\n")
            Path(header_directory, "sample.h").write_text(
                "static void *resolve(Address value, int offset) {\n"
                "    value.offset += offset;\n"
                "    return value.pointer;\n"
                "}\n"
            )
            counts = count_debt(Path(source_directory), Path(header_directory))

        self.assertEqual(counts["header_address_integer_arithmetic"], 1)

    def test_counts_byte_offset_members_in_game_headers(self):
        with tempfile.TemporaryDirectory() as source_directory, tempfile.TemporaryDirectory() as header_directory:
            Path(source_directory, "sample.c").write_text("void f(void) {}\n")
            Path(header_directory, "sample.h").write_text(
                "typedef union Address {\n"
                "    s32 byteOffset;\n"
                "    void *pointer;\n"
                "} Address;\n"
            )
            counts = count_debt(Path(source_directory), Path(header_directory))

        self.assertEqual(counts["header_byte_offset_members"], 1)


if __name__ == "__main__":
    unittest.main()
