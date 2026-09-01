/*
 * The team logo editor, swept.
 *
 * Two functions of two hundred lines each that nothing tested: mixing a colour
 * in the palette, and drawing with it on the canvas. Both are state machines
 * over about twenty globals driven by the pad, so rather than pick cases by
 * hand this walks the buttons across the states they act on and folds
 * everything the call could have changed into one number.
 */

#include "common.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/state.h"

#include <stdio.h>
#include <string.h>

/* The editor's own state. */
TeamLogoCanvas g_TeamLogoCanvas;
u16 g_TeamLogoClut[16];
TeamLogoColorIndex g_TeamLogoPenColor;
s32 g_TeamLogoBrushSize;
s32 g_TeamLogoColorChannel;
TeamLogoCoordinate g_TeamLogoCursorX;
s32 g_TeamLogoCursorY;
s32 g_TeamLogoDpadRepeatTimer;
s32 g_TeamLogoDpadRepeatMask;
u8 g_TeamLogoExpertMode;
s32 g_TeamLogoGuideMode;
s32 g_TeamLogoGuideModePrev;
s32 g_TeamLogoPaintArmed;
s32 g_TeamLogoPaletteMode;
TeamLogoCoordinate g_TeamLogoViewX;
s32 g_TeamLogoViewY;
volatile u16 g_PadHeld;
u16 g_PadPressed;
u16 g_PadPressedRepeat;

/* What the editor reaches outside itself. The canvas transforms are real
 * enough to move pixels, because whether one was called and in which direction
 * is part of what this is checking. */
static int s_cues;

void PlaySoundCue(s32 cue) { s_cues = s_cues * 31 + cue; }

static void ShiftCanvas(int dx, int dy) {
    TeamLogoCanvas copy = g_TeamLogoCanvas;
    int x, y;

    for (y = 0; y < 64; y++) {
        for (x = 0; x < 8; x++) {
            int sx = (x + dx) & 7;
            int sy = (y + dy) & 63;
            g_TeamLogoCanvas.words[y][x & 7] = copy.words[sy][sx & 7];
        }
    }
}

void ScrollTeamLogoUp(void) { ShiftCanvas(0, 1); }
void ScrollTeamLogoDown(void) { ShiftCanvas(0, -1); }
void ScrollTeamLogoLeft(void) { ShiftCanvas(1, 0); }
void ScrollTeamLogoRight(void) { ShiftCanvas(-1, 0); }
void FlipTeamLogoHorizontal(void) { ShiftCanvas(3, 0); }
void FlipTeamLogoVertical(void) { ShiftCanvas(0, 3); }
void RotateTeamLogoCw(void) { ShiftCanvas(1, 1); }
void RotateTeamLogoCcw(void) { ShiftCanvas(-1, -1); }

static unsigned long s_digest = 2166136261UL;

static int TestEditorControl(void) {
    memset(&g_TeamLogoCanvas, 0, sizeof(g_TeamLogoCanvas));
    g_TeamLogoPaletteMode = 0;
    g_TeamLogoExpertMode = 1;
    g_TeamLogoGuideMode = 2;
    g_TeamLogoGuideModePrev = 1;
    g_TeamLogoDpadRepeatTimer = 7;
    g_TeamLogoDpadRepeatMask = PAD_RIGHT;
    g_TeamLogoPaintArmed = 0;
    g_PadHeld = PAD_RIGHT;
    g_PadPressed = PAD_SELECT;

    UpdateTeamLogoCanvas();
    if (g_TeamLogoDpadRepeatTimer != 8 ||
        g_TeamLogoDpadRepeatMask != PAD_RIGHT ||
        g_TeamLogoGuideMode != 0 || g_TeamLogoGuideModePrev != 2 ||
        g_TeamLogoPaintArmed != 1) {
        puts("FAIL team logo editor control state");
        return 0;
    }

    g_TeamLogoExpertMode = 0;
    g_TeamLogoGuideMode = 2;
    g_TeamLogoDpadRepeatTimer = 5;
    g_TeamLogoDpadRepeatMask = PAD_LEFT;
    g_PadHeld = 0;
    g_PadPressed = 0;
    UpdateTeamLogoCanvas();
    if (g_TeamLogoGuideMode != 1 || g_TeamLogoDpadRepeatTimer != 0 ||
        g_TeamLogoDpadRepeatMask != 0) {
        puts("FAIL team logo editor idle control state");
        return 0;
    }
    return 1;
}

static void Fold(FILE *out, const char *label) {
    char line[320];
    const char *p;
    unsigned long canvas = 0;
    int i;

    for (i = 0; i < (int)sizeof(g_TeamLogoCanvas); i++) {
        canvas = (canvas ^ ((const u8 *)&g_TeamLogoCanvas)[i]) * 16777619UL;
        canvas &= 0xFFFFFFFFUL;
    }
    snprintf(line, sizeof(line),
             "%s pen=%d channel=%d brush=%d cursor=%d,%d view=%d,%d "
             "repeat=%d expert=%d guide=%d/%d armed=%d palette=%d "
             "clut=%04x,%04x,%04x canvas=%08lx cues=%d\n",
             label, (int)g_TeamLogoPenColor, g_TeamLogoColorChannel,
             g_TeamLogoBrushSize, (int)g_TeamLogoCursorX, g_TeamLogoCursorY,
             (int)g_TeamLogoViewX, g_TeamLogoViewY, g_TeamLogoDpadRepeatTimer,
             (int)g_TeamLogoExpertMode, g_TeamLogoGuideMode, g_TeamLogoGuideModePrev,
             g_TeamLogoPaintArmed, g_TeamLogoPaletteMode, g_TeamLogoClut[0],
             g_TeamLogoClut[1], g_TeamLogoClut[15], canvas, s_cues);
    for (p = line; *p != '\0'; p++) {
        s_digest = (s_digest ^ (unsigned char)*p) * 16777619UL;
        s_digest &= 0xFFFFFFFFUL;
    }
    if (out != NULL) {
        fputs(line, out);
    }
}

int main(int argc, char **argv) {
    /*
     * What the editor did before it was taken apart. Run the test with a file
     * name to write the sweep out and diff two runs when this moves.
     */
    static const unsigned long expected = 1059771453UL;
    static const u16 buttons[] = {
        0x0000, 0x0010, 0x0020, 0x0040, 0x0080, 0x1000, 0x2000, 0x4000,
        0x8000, 0x0001, 0x0002, 0x0004, 0x0008, 0x0100, 0x0200, 0x0060,
    };
    FILE *out = NULL;
    size_t held, pressed;
    int expert, palette, channel, brush, repeat, steps = 0;
    char label[96];

    if (!TestEditorControl()) return 1;

    if (argc > 1) {
        out = fopen(argv[1], "w");
        if (out == NULL) {
            printf("cannot write %s\n", argv[1]);
            return 1;
        }
    }

    for (expert = 0; expert < 2; expert++) {
        for (palette = 0; palette < 2; palette++) {
            for (channel = 0; channel < 3; channel++) {
                for (brush = 0; brush < 3; brush++) {
                    /* 1 and 0x14 are the two the d-pad repeat tests for by name. */
                    static const s32 repeats[] = {0, 1, 0xA, 0x14};
                    for (repeat = 0; repeat < 4; repeat++) {
                        for (held = 0;
                             held < sizeof(buttons) / sizeof(buttons[0]);
                             held++) {
                            for (pressed = 0;
                                 pressed < sizeof(buttons) / sizeof(buttons[0]);
                                 pressed++) {
                                int i;

                                /* Each step starts from a canvas with enough
                                 * in it that a scroll or a plot shows. */
                                memset(&g_TeamLogoCanvas, 0,
                                       sizeof(g_TeamLogoCanvas));
                                for (i = 0; i < 64; i++) {
                                    g_TeamLogoCanvas.words[i][i & 7] =
                                        (u32)(0x12345678u + (u32)i);
                                }
                                for (i = 0; i < 16; i++) {
                                    g_TeamLogoClut[i] = (u16)(0x0421 * i);
                                }
                                g_TeamLogoPenColor = (TeamLogoColorIndex)3;
                                g_TeamLogoColorChannel = channel;
                                g_TeamLogoBrushSize = brush;
                                g_TeamLogoCursorX = (TeamLogoCoordinate)20;
                                g_TeamLogoCursorY = 30;
                                g_TeamLogoViewX = (TeamLogoCoordinate)4;
                                g_TeamLogoViewY = 6;
                                g_TeamLogoDpadRepeatTimer = repeats[repeat];
                                g_TeamLogoExpertMode = (u8)expert;
                                g_TeamLogoGuideMode = 1;
                                g_TeamLogoGuideModePrev = 0;
                                g_TeamLogoPaintArmed = 1;
                                g_TeamLogoPaletteMode = palette;
                                g_PadHeld = buttons[held];
                                g_PadPressed = buttons[pressed];
                                g_PadPressedRepeat = buttons[pressed];
                                s_cues = 0;

                                if (palette == 1) {
                                    EditLogoPalette();
                                } else {
                                    EditLogoCanvas();
                                }

                                sprintf(label,
                                        "e%d/p%d/c%d/b%d/r%d/h%04x/x%04x",
                                        expert, palette, channel, brush,
                                        repeats[repeat], buttons[held],
                                        buttons[pressed]);
                                Fold(out, label);
                                steps++;
                            }
                        }
                    }
                }
            }
        }
    }

    if (out != NULL) {
        fclose(out);
    }
    if (s_digest != expected) {
        printf("FAIL the logo editor behaves differently: %d steps digest to "
               "%lu, expected %lu\n", steps, s_digest, expected);
        return 1;
    }
    printf("the logo editor takes the same %d steps it always did\n", steps);
    return 0;
}
