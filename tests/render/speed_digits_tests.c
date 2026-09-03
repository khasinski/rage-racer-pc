#include <assert.h>
#include <limits.h>
#include <string.h>

#include "game/car.h"
#include "game/race.h"
#include "game/race_hud_internal.h"
#include "game/render_internal.h"

typedef struct DigitCall {
    u8 *packet;
    s32 x;
    s32 y;
    s32 digit;
    u16 clut;
} DigitCall;

static GameCarSpec s_CarSpec;
static GameFrameContext s_Frame;
static DigitCall s_Digits[3];
static s32 s_DigitCount;
static GameOrderingTableEntry *s_DrawModeOt;
static s32 s_DrawModeTpage;

GameCarSpec *g_CarSpec = &s_CarSpec;
GameFrameContext *g_DrawBuffer = &s_Frame;
GameRenderState g_RenderState;
u16 g_HudGlyphClut;

u8 *DrawHudDigit(u8 *packet, s32 x, s32 y, s32 digit, u16 clut) {
    assert(s_DigitCount < 3);
    s_Digits[s_DigitCount++] = (DigitCall){packet, x, y, digit, clut};
    return packet + 8;
}

u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *packet, s32 tpage) {
    s_DrawModeOt = ot;
    s_DrawModeTpage = tpage;
    return packet + 4;
}

static void CheckSpeed(s32 value, s32 hundreds, s32 tens, s32 ones) {
    (void)hundreds;
    (void)ones;
    (void)tens;
    u8 packets[64];

    memset(packets, 0, sizeof(packets));
    s_DigitCount = 0;
    s_DrawModeOt = NULL;
    s_DrawModeTpage = -1;
    g_RenderState.packetCursor = packets;

    DrawSpeedDigits(10, 20, value);

    assert(s_DigitCount == 3);
    assert(s_Digits[0].packet == packets);
    assert(s_Digits[1].packet == packets + 8);
    assert(s_Digits[2].packet == packets + 16);
    assert(s_Digits[0].x == 110 && s_Digits[1].x == 118 &&
           s_Digits[2].x == 126);
    assert(s_Digits[0].y == 220 && s_Digits[1].y == 220 &&
           s_Digits[2].y == 220);
    assert(s_Digits[0].digit == hundreds);
    assert(s_Digits[1].digit == tens);
    assert(s_Digits[2].digit == ones);
    assert(s_Digits[0].clut == 0x456 && s_Digits[1].clut == 0x456 &&
           s_Digits[2].clut == 0x456);
    assert(s_DrawModeOt == GamePrimaryOrderingTable(0));
    assert(s_DrawModeTpage == 9);
    assert(g_RenderState.packetCursor == packets + 28);
}

int main(void) {
    u8 packets[64];

    memset(&s_CarSpec, 0, sizeof(s_CarSpec));
    memset(&s_Frame, 0, sizeof(s_Frame));
    s_CarSpec.tachometer.digitsX = 100;
    s_CarSpec.tachometer.digitsY = 200;
    g_HudGlyphClut = 0x456;

    CheckSpeed(0, 0, 0, 0);
    CheckSpeed(7, 0, 0, 7);
    CheckSpeed(160, 1, 6, 0);
    CheckSpeed(999, 9, 9, 9);
    CheckSpeed(-1, 0, 0, 0);
    CheckSpeed(1000, 9, 9, 9);

    memset(packets, 0, sizeof(packets));
    s_DigitCount = 0;
    g_RenderState.packetCursor = packets;
    s_CarSpec.tachometer.digitsX = 1;
    s_CarSpec.tachometer.digitsY = 1;
    DrawSpeedDigits(INT_MAX, INT_MAX, 0);
    assert(s_Digits[0].x == INT_MIN && s_Digits[1].x == INT_MIN + 8 &&
           s_Digits[2].x == INT_MIN + 16);
    assert(s_Digits[0].y == INT_MIN && s_Digits[1].y == INT_MIN &&
           s_Digits[2].y == INT_MIN);
    return 0;
}
