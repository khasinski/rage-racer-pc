#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rage/jp_text.h"

enum {
    BASE = 0x80010000u,
    SIZE = 0x800 + 0x400,
    GLYPHS = 0x100, /* address offset of the glyph runs */
    TABLE = 0x180,  /* address offset of the pointer table */
};

static int s_failures;

static void Expect(const char *what, int actual, int expected) {
    if (actual != expected) {
        fprintf(stderr, "%s: got %d, expected %d\n", what, actual, expected);
        s_failures++;
    }
}

static void Put32(unsigned char *exe, unsigned int address, unsigned int v) {
    unsigned char *p = exe + 0x800 + (address - BASE);
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
    p[2] = (unsigned char)(v >> 16);
    p[3] = (unsigned char)(v >> 24);
}

static void Put16(unsigned char *exe, unsigned int address, int v) {
    unsigned char *p = exe + 0x800 + (address - BASE);
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)((unsigned int)v >> 8);
}

/* A layout shaped like SLPS_006.00: message 0 is a two-strip run onto the
 * second line, 1..15 share one strip, and 16..18 are the three banners. The
 * column table overlaps the last pointer, as in retail. */
static unsigned char *BuildExe(void) {
    unsigned char *exe = calloc(1, SIZE);
    static const unsigned char run0[] = {148, 48, 36, 2, 0, 120, 172, 0};
    static const unsigned char other[] = {10, 20, 30, 0};
    static const unsigned char banners[] = {
        0, 0, 108, 0, 0, 24, 108, 0, 0, 48, 108, 0,
    };
    unsigned int g = BASE + GLYPHS;
    int message;

    memcpy(exe, "PS-X EXE", 8);
    exe[0x18] = (unsigned char)BASE;
    exe[0x19] = (unsigned char)(BASE >> 8);
    exe[0x1A] = (unsigned char)(BASE >> 16);
    exe[0x1B] = (unsigned char)(BASE >> 24);
    memcpy(exe + 0x800 + GLYPHS, run0, sizeof(run0));
    memcpy(exe + 0x800 + GLYPHS + 8, other, sizeof(other));
    memcpy(exe + 0x800 + GLYPHS + 12, banners, sizeof(banners));
    for (message = 0; message < 20; message++) {
        unsigned int row = message == 0 ? g
                         : message >= 16 && message <= 18
                               ? g + 12 + (unsigned int)(message - 16) * 4
                               : g + 8;
        Put32(exe, BASE + TABLE + (unsigned int)message * 4, row);
    }
    Put16(exe, BASE + TABLE + 0x4C + 4, 96);
    Put16(exe, BASE + TABLE + 0x4C + 6, 120);
    Put16(exe, BASE + TABLE + 0x4C + 8, 180);
    return exe;
}

int main(void) {
    unsigned char *exe = BuildExe();
    const JpTextGlyph *row;

    Expect("nothing adopted at start", JpTextActive(), 0);
    Expect("finds the table", JpTextAdoptExecutable(exe, SIZE), 1);
    Expect("Japanese active", JpTextActive(), 1);
    Expect("memory card active", JpTextMemoryCardActive(), 1);
    row = JpTextMemoryCardRow(0);
    Expect("row 0 present", row != NULL, 1);
    if (row != NULL) {
        Expect("row 0 u", row[0].u, 148);
        Expect("row 0 width", row[0].width, 36);
        Expect("row 0 column", row[0].column, 2);
        Expect("row 0 second v", row[1].v, 120);
        Expect("row 0 ends", row[1].column, 0);
    }
    row = JpTextMemoryCardRow(17);
    Expect("banner v", row != NULL ? row[0].v : -1, 24);
    Expect("column 2", JpTextMemoryCardColumnX(2), 96);
    Expect("column 4", JpTextMemoryCardColumnX(4), 180);
    Expect("out of range row", JpTextMemoryCardRow(20) == NULL, 1);

    /* A PAL-shaped executable has no banner strips: Japanese drawing stays
     * off for the memory card, though the disc is still Japanese. */
    memset(exe + 0x800 + GLYPHS + 12, 0x55, 12);
    Expect("no table", JpTextAdoptExecutable(exe, SIZE), 0);
    Expect("memory card inactive", JpTextMemoryCardActive(), 0);
    Expect("row withheld", JpTextMemoryCardRow(0) == NULL, 1);

    Expect("not an executable", JpTextAdoptExecutable(exe + 1, SIZE - 1), 0);
    Expect("inactive after reject", JpTextActive(), 0);
    JpTextReset();
    Expect("reset", JpTextActive(), 0);
    free(exe);
    if (s_failures == 0) puts("jp_text: all checks passed");
    return s_failures != 0;
}
