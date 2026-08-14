import unittest

from tools.scripts.strip_nonmatching_markers import strip


class StripTest(unittest.TestCase):
    def test_removes_the_marker_and_leaves_the_symbol(self):
        text = "nonmatching g_AtanTable\n\ndlabel g_AtanTable\n    .short 0x0000\n"
        self.assertEqual(strip(text), "\ndlabel g_AtanTable\n    .short 0x0000\n")

    def test_removes_the_sized_form(self):
        self.assertEqual(strip("nonmatching Lzc, 0x18\nglabel Lzc\n"), "glabel Lzc\n")

    def test_leaves_instruction_words_alone(self):
        # The build has to come out byte-identical, so nothing but the marker
        # line may go.
        text = "glabel Fn\n    /* 1234 80010000 03E00008 */  jr $ra\n"
        self.assertEqual(strip(text), text)

    def test_does_not_touch_an_indented_mention(self):
        # An instruction operand or a comment that happens to contain the word
        # is not a marker; only a line that starts with one is.
        text = "    addiu $a0, $zero, 0x2 /* nonmatching */\n"
        self.assertEqual(strip(text), text)

    def test_is_idempotent(self):
        once = strip("nonmatching Fn\nglabel Fn\n")
        self.assertEqual(strip(once), once)


if __name__ == "__main__":
    unittest.main()
