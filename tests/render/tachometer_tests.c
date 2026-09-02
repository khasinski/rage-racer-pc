#include "game/car.h"
#include "game/diagnostics.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"

#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;
static GameFrameContext s_frame;
GameFrameContext *g_DrawBuffer = &s_frame;
static GameCarSpec s_carSpec;
GameCarSpec *g_CarSpec = &s_carSpec;
PlayerCarRuntime g_PlayerCar;
GameSpriteDesc g_TachoNeedleSprite;
s16 g_TachoNeedleQuad[4][2];
u16 g_HudGlyphClut;
u8 g_TachoFaceR;
u8 g_TachoFaceG;
u8 g_TachoFaceB;

static s32 s_sineAngle;
static s32 s_cosineAngle;
static u8 *s_digitPacket;
static s32 s_digitX;
static s32 s_digitY;
static s32 s_digit;
static u16 s_digitClut;
static s32 s_speedX;
static s32 s_speedY;
static s32 s_speed;

s32 DiagnosticsEnabled(const char *name) {
    (void)name;
    return 0;
}

int HudRightX(int x) {
    return x + 100;
}

s32 rsin(s32 angle) {
    s_sineAngle = angle;
    return 0;
}

s32 rcos(s32 angle) {
    s_cosineAngle = angle;
    return 4096;
}

u8 *DrawHudDigit(u8 *packet, s32 x, s32 y, s32 digit, u16 clut) {
    s_digitPacket = packet;
    s_digitX = x;
    s_digitY = y;
    s_digit = digit;
    s_digitClut = clut;
    return packet + sizeof(SPRT_8);
}

void DrawSpeedDigits(s32 centerX, s32 centerY, s32 speed) {
    s_speedX = centerX;
    s_speedY = centerY;
    s_speed = speed;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__,   \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void ResetState(u8 *packets) {
    memset(&s_frame, 0, sizeof(s_frame));
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    g_RenderState.packetCursor = packets;
    s_digitPacket = NULL;
    s_speed = -1;
}

int main(void) {
    u8 packets[512];
    CarTachometerSpec *spec = &g_CarSpec->tachometer;
    POLY_F4 *needle;
    TILE *shiftLight;
    GameFrameContext *frame = g_DrawBuffer;

    memset(&s_carSpec, 0, sizeof(s_carSpec));
    spec->needleX = 20;
    spec->needleY = 30;
    spec->gearDigitDX = 4;
    spec->gearDigitDY = 5;
    spec->shiftLightDX = 6;
    spec->shiftLightDY = 7;
    spec->angleMin = 100;
    spec->angleMax = 1100;
    spec->needleColor[0] = 10;
    spec->needleColor[1] = 20;
    spec->needleColor[2] = 30;
    spec->needleColorAlt[0] = 40;
    spec->needleColorAlt[1] = 50;
    spec->needleColorAlt[2] = 60;
    g_TachoNeedleQuad[0][0] = -2;
    g_TachoNeedleQuad[0][1] = -3;
    g_TachoNeedleQuad[1][0] = 2;
    g_TachoNeedleQuad[1][1] = -3;
    g_TachoNeedleQuad[2][0] = -2;
    g_TachoNeedleQuad[2][1] = 3;
    g_TachoNeedleQuad[3][0] = 2;
    g_TachoNeedleQuad[3][1] = 3;
    g_TachoNeedleSprite.x = 12;
    g_HudGlyphClut = 0x456;

    memset(packets, 0, sizeof(packets));
    ResetState(packets);
    g_PlayerCar.drive.gear = 3;
    g_PlayerCar.speed = 1168;
    DrawTachometer(5000, 1, TACHOMETER_LIGHTING_NORMAL, 0);

    needle = (POLY_F4 *)packets;
    CHECK(s_sineAngle == 600 && s_cosineAngle == 600);
    CHECK(needle->x0 == 118 && needle->y0 == 27);
    CHECK(needle->x1 == 122 && needle->y1 == 27);
    CHECK(needle->x2 == 118 && needle->y2 == 33);
    CHECK(needle->x3 == 122 && needle->y3 == 33);
    CHECK(needle->r0 == 10 && needle->g0 == 20 && needle->b0 == 30);
    CHECK(s_digitPacket == packets + sizeof(POLY_F4));
    CHECK(s_digitX == 124 && s_digitY == 35 && s_digit == 3);
    CHECK(s_digitClut == 0x456);
    CHECK(s_speedX == 120 && s_speedY == 30 && s_speed == 160);
    CHECK(frame->layout.raceHud.tachometerFace.r0 == 0x80);
    CHECK(frame->layout.raceHud.tachometerFace.clut == 0x33A8);
    CHECK(frame->layout.raceHud.tachometerFace.x0 == 112);
    shiftLight = (TILE *)(packets + sizeof(POLY_F4) + sizeof(SPRT_8));
    CHECK(shiftLight->x0 == 126 && shiftLight->y0 == 37);
    CHECK(shiftLight->w == 16 && shiftLight->h == 16);
    CHECK(shiftLight->r0 == (u8)255 && shiftLight->g0 == 32 &&
          shiftLight->b0 == 32);
    CHECK(g_RenderState.packetCursor == (u8 *)(shiftLight + 1));

    memset(packets, 0, sizeof(packets));
    ResetState(packets);
    DrawTachometer(0, 0, TACHOMETER_LIGHTING_DARK, 0);
    needle = (POLY_F4 *)packets;
    CHECK(needle->r0 == 40 && needle->g0 == 50 && needle->b0 == 60);
    CHECK(frame->layout.raceHud.tachometerFace.clut == 0x33E8);

    memset(packets, 0, sizeof(packets));
    ResetState(packets);
    DrawTachometer(0, 0, TACHOMETER_LIGHTING_FADE_TO_DARK, 200);
    needle = (POLY_F4 *)packets;
    CHECK(needle->r0 == 32 && needle->g0 == 32 && needle->b0 == 32);
    CHECK(g_TachoFaceR == 32);

    memset(packets, 0, sizeof(packets));
    ResetState(packets);
    DrawTachometer(0, 0, TACHOMETER_LIGHTING_FADE_FROM_DARK, 32);
    needle = (POLY_F4 *)packets;
    CHECK(needle->r0 == 32 && needle->g0 == 32 && needle->b0 == 32);
    CHECK(g_TachoFaceR == 32);

    memset(packets, 0, sizeof(packets));
    ResetState(packets);
    DrawTachometer(0, 0, TACHOMETER_LIGHTING_FADE_TO_DARK, 48);
    needle = (POLY_F4 *)packets;
    CHECK(needle->r0 == 21 && needle->g0 == 26 && needle->b0 == 31);
    CHECK(g_TachoFaceR == 80);

    memset(packets, 0, sizeof(packets));
    ResetState(packets);
    DrawTachometer(0, 0, TACHOMETER_LIGHTING_FADE_FROM_DARK, 80);
    needle = (POLY_F4 *)packets;
    CHECK(needle->r0 == 21 && needle->g0 == 26 && needle->b0 == 31);
    CHECK(g_TachoFaceR == 80);

    puts("tachometer tests passed");
    return 0;
}
