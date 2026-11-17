#ifndef RAGE_JP_TEXT_H
#define RAGE_JP_TEXT_H

/* Text drawn the Japanese way.
 *
 * The PAL code sets its messages letter by letter from an alphabet sheet in
 * VRAM. The Japanese release keeps whole pre-drawn Japanese phrases in that
 * same place instead and draws each message as a few sprites cut from it, so
 * on a Japanese disc the PAL messages come out blank. When the mounted disc is
 * Japanese, the port reads the layout tables off that disc's own executable
 * and the drawing code below the game uses them the way the Japanese build
 * did. Nothing here is compiled-in game data; with any other disc, or when the
 * tables cannot be found, everything stays as the PAL build draws it. */

#include <stddef.h>

typedef struct JpTextGlyph {
    unsigned char u;
    unsigned char v;
    unsigned char width;
    unsigned char column; /* 0 ends the message, 1 continues the line,
                             2.. moves to the second line at that column */
} JpTextGlyph;

/* Scans a PS-X EXE image for the Japanese tables. Returns 1 when found. */
int JpTextAdoptExecutable(const unsigned char *exe, size_t size);
void JpTextReset(void);

/* 1 when the mounted disc is a Japanese one (its executable was read).
 * Screens whose Japanese layout is fixed in code switch on this alone. */
int JpTextActive(void);

/* 1 once a Japanese executable's memory card table has been adopted. */
int JpTextMemoryCardActive(void);
/* The glyph run of a memory card message, or NULL. */
const JpTextGlyph *JpTextMemoryCardRow(int message);
/* X of a second-line column (2..4). */
int JpTextMemoryCardColumnX(int column);

#endif
