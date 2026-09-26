#include "game/car.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;
static GameFrameContext s_frame;
GameFrameContext *g_DrawBuffer = &s_frame;
static GameCarSpec s_carSpec;
static s32 s_gear, s_speedInput;

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
static u16 s_speedColor;
static const char *s_region = "PAL";
const char *HostDiscRegion(void) { return s_region; }

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

void DrawSpeedDigits(s32 centerX, s32 centerY, s32 speed, u16 color) {
    s_speedColor = color;
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
    s_gear = s_speedInput = 0;
    g_RenderState.draw.packetCursor = packets;
    s_digitPacket = NULL;
    s_speed = -1;
}

int main(void) {
    u8 packets[512];
    CarTachometerSpec *spec = &s_carSpec.tachometer;
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
    spec->needleQuad[0] = 3; spec->needleQuad[1] = 2;
    spec->needleQuad[2] = 3; spec->needleQuad[3] = 2;
    spec->faceDX = -8;

    memset(packets, 0, sizeof(packets));
    ResetState(packets);
    s_gear = 3;
    s_speedInput = 1168;
    DrawTachometer(spec, 1, s_gear, s_speedInput, 5000, 1, TACHOMETER_LIGHTING_NORMAL, 0);

    needle = (POLY_F4 *)packets;
    CHECK(s_sineAngle == 600 && s_cosineAngle == 600);
    CHECK(needle->x0 == 118 && needle->y0 == 33);
    CHECK(needle->x1 == 118 && needle->y1 == 27);
    CHECK(needle->x2 == 122 && needle->y2 == 33);
    CHECK(needle->x3 == 122 && needle->y3 == 27);
    CHECK(needle->r0 == 10 && needle->g0 == 20 && needle->b0 == 30);
    CHECK(s_digitPacket == packets + sizeof(POLY_F4));
    CHECK(s_digitX == 124 && s_digitY == 35 && s_digit == 3);
    CHECK(s_digitClut == 0x7800);
    CHECK(s_speedX == 120 && s_speedY == 30 && s_speed == 160);
    CHECK(frame->layout.raceHud.tachometerFace.r0 == 0x80);
    CHECK(frame->layout.raceHud.tachometerFace.clut == 0x33A8);
    CHECK(frame->layout.raceHud.tachometerFace.x0 == 112);
    shiftLight = (TILE *)(packets + sizeof(POLY_F4) + sizeof(SPRT_8));
    CHECK(shiftLight->x0 == 126 && shiftLight->y0 == 37);
    CHECK(shiftLight->w == 16 && shiftLight->h == 16);
    CHECK(shiftLight->r0 == (u8)255 && shiftLight->g0 == 32 &&
          shiftLight->b0 == 32);
    CHECK(g_RenderState.draw.packetCursor == (u8 *)(shiftLight + 1));

    memset(packets, 0, sizeof(packets));
    ResetState(packets);
    DrawTachometer(spec, 1, s_gear, s_speedInput, 0, 0, TACHOMETER_LIGHTING_DARK, 0);
    needle = (POLY_F4 *)packets;
    CHECK(needle->r0 == 40 && needle->g0 == 50 && needle->b0 == 60);
    CHECK(frame->layout.raceHud.tachometerFace.clut == 0x33E8);

    memset(packets, 0, sizeof(packets));
    ResetState(packets);
    DrawTachometer(spec, 1, s_gear, s_speedInput, 0, 0, TACHOMETER_LIGHTING_FADE_TO_DARK, 200);
    needle = (POLY_F4 *)packets;
    CHECK(needle->r0 == 32 && needle->g0 == 32 && needle->b0 == 32);
    CHECK(frame->layout.raceHud.tachometerFace.r0 == 32);

    memset(packets, 0, sizeof(packets));
    ResetState(packets);
    DrawTachometer(spec, 1, s_gear, s_speedInput, 0, 0, TACHOMETER_LIGHTING_FADE_FROM_DARK, 32);
    needle = (POLY_F4 *)packets;
    CHECK(needle->r0 == 32 && needle->g0 == 32 && needle->b0 == 32);
    CHECK(frame->layout.raceHud.tachometerFace.r0 == 32);

    memset(packets, 0, sizeof(packets));
    ResetState(packets);
    DrawTachometer(spec, 1, s_gear, s_speedInput, 0, 0, TACHOMETER_LIGHTING_FADE_TO_DARK, 48);
    needle = (POLY_F4 *)packets;
    CHECK(needle->r0 == 21 && needle->g0 == 26 && needle->b0 == 31);
    CHECK(frame->layout.raceHud.tachometerFace.r0 == 80);

    memset(packets, 0, sizeof(packets));
    ResetState(packets);
    DrawTachometer(spec, 1, s_gear, s_speedInput, 0, 0, TACHOMETER_LIGHTING_FADE_FROM_DARK, 80);
    needle = (POLY_F4 *)packets;
    CHECK(needle->r0 == 21 && needle->g0 == 26 && needle->b0 == 31);
    CHECK(frame->layout.raceHud.tachometerFace.r0 == 80);

    memset(packets, 0, sizeof(packets));
    ResetState(packets);
    s_speedInput = INT_MAX;
    DrawTachometer(spec, 1, s_gear, s_speedInput, INT_MAX, 0, TACHOMETER_LIGHTING_NORMAL, 0);
    CHECK(s_sineAngle == 1100 && s_cosineAngle == 1100);
    CHECK(s_speed == 999);

    memset(packets, 0, sizeof(packets));
    ResetState(packets);
    s_speedInput = INT_MIN;
    DrawTachometer(spec, 1, s_gear, s_speedInput, INT_MIN, 0, TACHOMETER_LIGHTING_NORMAL, 0);
    CHECK(s_sineAngle == 100 && s_cosineAngle == 100);
    CHECK(s_speed == 0);

    memset(packets, 0, sizeof(packets));
    ResetState(packets);
    DrawTachometer(spec, 1, s_gear, s_speedInput, 0, 0, TACHOMETER_LIGHTING_FADE_FROM_DARK, INT_MIN);
    needle = (POLY_F4 *)packets;
    CHECK(needle->r0 == 32 && needle->g0 == 32 && needle->b0 == 32);
    CHECK(frame->layout.raceHud.tachometerFace.r0 == 32);

    memset(packets, 0, sizeof(packets));
    ResetState(packets);
    spec->shiftLightDX = UINT16_MAX;
    spec->shiftLightDY = UINT16_MAX;
    DrawTachometer(spec, 1, s_gear, s_speedInput, 0, 0, TACHOMETER_LIGHTING_NORMAL, 0);
    shiftLight = (TILE *)(packets + sizeof(POLY_F4) + sizeof(SPRT_8));
    CHECK(shiftLight->x0 == 119 && shiftLight->y0 == 29);

    /* US artwork says mph. The numeric value must use the retail US
     * conversion, without changing the simulation speed it is reading. */
    const char *regions[] = {"PAL", "NTSC-U", "NTSC-J", "unknown", NULL};
    const int speeds[] = {INT_MIN, -1, 0, 1, 14, 1168, 1175, 7300, INT_MAX};
    const int metric[] = {0, 0, 0, 0, 1, 160, 160, 999, 999};
    const int imperial[] = {0, 0, 0, 0, 0, 100, 100, 625, 999};
    for (unsigned r = 0; r < sizeof(regions)/sizeof(regions[0]); ++r) {
        s_region = regions[r];
        for (unsigned v = 0; v < sizeof(speeds)/sizeof(speeds[0]); ++v) {
            memset(packets, 0, sizeof(packets));
            ResetState(packets);
            s_speedInput = speeds[v];
            CarTachometerSpec before = *spec;
            DrawTachometer(spec, 1, s_gear, s_speedInput, 0, 0, TACHOMETER_LIGHTING_NORMAL, 0);
            CHECK(s_speed == (r == 1 ? imperial[v] : metric[v]));
            CHECK(memcmp(&before, spec, sizeof(before)) == 0 && s_speedInput == speeds[v]);
        }
    }
/* A second seat supplies a different dial without installing car globals. */
CarTachometerSpec other = *spec;
other.digitsX = 9; other.digitsY = 3;
other.needleX = 7; other.needleY = 8; other.faceDX = 2; other.faceDY = 3;
other.needleQuad[0] = 3; other.needleQuad[1] = 4;
other.needleQuad[2] = 5; other.needleQuad[3] = 6;
memset(packets, 0, sizeof(packets));
ResetState(packets);
DrawTachometer(&other, 0, 6, 1460, 0, 0, TACHOMETER_LIGHTING_NORMAL, 0);
needle = (POLY_F4 *)packets;
CHECK(needle->x0 == 101 && needle->y0 == 13);
CHECK(s_digit == 6 && s_speed == 200);
CHECK(s_digitClut == 0x78CF && s_speedColor == 0x78CF);
CHECK(s_speedX == 116 && s_speedY == 11);
CHECK(frame->layout.raceHud.tachometerFace.x0 == 109 &&
      frame->layout.raceHud.tachometerFace.y0 == 11);
memset(packets, 0, sizeof(packets));
ResetState(packets);
DrawTachometer(spec, 1, 3, 1168, 0, 0, TACHOMETER_LIGHTING_NORMAL, 0);
needle = (POLY_F4 *)packets;
CHECK(needle->x0 == 118 && needle->y0 == 33);
CHECK(s_digit == 3 && s_speed == 160);
CHECK(s_digitClut == 0x7800 && s_speedColor == 0x7800);
CHECK(frame->layout.raceHud.tachometerFace.x0 == 112 &&
      frame->layout.raceHud.tachometerFace.y0 == 30);
    puts("tachometer tests passed");
    return 0;
}
