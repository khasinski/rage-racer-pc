#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/render.h"
#include "game/render_internal.h"

GameRenderState g_RenderState;
u8 g_DrawModeEnv[8];
ProportionalFontCell g_PropFontCells[PROPORTIONAL_FONT_CELL_COUNT];
u8 g_WordFontCells[40];
u8 g_HighFontCell[4];

static GameFrameContext s_frame;
GameFrameContext *g_DrawBuffer = &s_frame;
static int s_failures;

#define CHECK_EQ(actual, expected, label) do { \
    s32 actualValue = (s32)(actual); \
    s32 expectedValue = (s32)(expected); \
    if (actualValue != expectedValue) { \
        printf("FAIL %s: got %d, expected %d\n", \
               label, actualValue, expectedValue); \
        s_failures++; \
    } \
} while (0)

static void ResetTextState(void) {
    memset(&s_frame, 0, sizeof(s_frame));
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    memset(g_PropFontCells, 0, sizeof(g_PropFontCells));
    memset(g_WordFontCells, 0, sizeof(g_WordFontCells));
    memset(g_HighFontCell, 0, sizeof(g_HighFontCell));
    g_RenderState.packetCursor = s_frame.layout.primitiveBuffer;
}

static void CheckFontClasses(void) {
    u8 *packets;
    SPRT *fixed;
    SPRT *word;
    SPRT *high;
    SPRT *afterCurrency;

    ResetTextState();
    packets = g_RenderState.packetCursor;
    g_PropFontCells['A' - 0x20].textureU = 11;
    g_PropFontCells['A' - 0x20].textureV = 12;
    g_WordFontCells[0] = 21;
    g_WordFontCells[1] = 22;
    g_WordFontCells[2] = 7;
    g_WordFontCells[3] = 9;
    g_HighFontCell[0] = 31;
    g_HighFontCell[1] = 32;
    g_HighFontCell[2] = 8;
    g_HighFontCell[3] = 3;

    DrawProportionalText(10, 20, "A avA", 0x1234);
    fixed = (SPRT *)packets;
    word = fixed + 1;
    high = word + 1;
    afterCurrency = high + 1;

    CHECK_EQ(fixed->x0, 10, "fixed x");
    CHECK_EQ(fixed->y0, 20, "fixed y");
    CHECK_EQ(fixed->u0, 11, "fixed u");
    CHECK_EQ(fixed->v0, 12, "fixed v");
    CHECK_EQ(fixed->w, 12, "fixed width");
    CHECK_EQ(word->x0, 34, "word x after space");
    CHECK_EQ(word->u0, 21, "word u");
    CHECK_EQ(word->w, 7, "word width");
    CHECK_EQ(high->x0, 43, "high x after advance");
    CHECK_EQ(high->y0, 23, "high y offset");
    CHECK_EQ(high->u0, 31, "high u");
    CHECK_EQ(high->w, 8, "high width");
    CHECK_EQ(afterCurrency->x0, 50,
             "currency marker uses the retail word-cell advance");
    CHECK_EQ(afterCurrency->u0, 11, "fixed glyph after currency");
    CHECK_EQ(g_RenderState.packetCursor ==
                 packets + 4 * sizeof(SPRT) + sizeof(DrawPacket),
             1,
             "packet cursor");
}

static void CheckShadedText(void) {
    SPRT *sprite;

    ResetTextState();
    g_PropFontCells['A' - 0x20].textureU = 5;
    g_PropFontCells['A' - 0x20].textureV = 6;
    sprite = (SPRT *)g_RenderState.packetCursor;

    GameDrawProportionalTextShaded(1, 2, "A", 3, 0x45);
    CHECK_EQ(sprite->r0, 0x45, "shade red");
    CHECK_EQ(sprite->g0, 0x45, "shade green");
    CHECK_EQ(sprite->b0, 0x45, "shade blue");
    CHECK_EQ((sprite->code & 2) != 0, 1, "shade semitransparency");
}

static void CheckInvalidFontCodesAreSkipped(void) {
    static const char invalid[] = {'`', 'k', 'w', (char)0x80, '\0'};
    u8 *packets;

    ResetTextState();
    packets = g_RenderState.packetCursor;
    DrawProportionalText(10, 20, invalid, 3);

    CHECK_EQ(g_RenderState.packetCursor == packets + sizeof(DrawPacket), 1,
             "invalid font codes only queue draw mode");
}

int main(void) {
    CheckFontClasses();
    CheckShadedText();
    CheckInvalidFontCodesAreSkipped();
    return s_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
