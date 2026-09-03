#include "game/input_internal.h"
#include "game/menu.h"
#include "game/prim.h"
#include "game/render_internal.h"

#include <stdio.h>
#include <string.h>

GameFrameContext *g_DrawBuffer;
GameRenderState g_RenderState;
NegconCalibrationValue g_NegconMaxTwist;
NegconCalibrationValue g_NegconSteerPlay;
s16 g_NegconPlayPercent[NEGCON_CALIBRATION_COUNT] = {0, 3, 5, 7};
char g_MsgNegconMaxTwist[] = "MAX";
char g_MsgNegconSteerPlay[] = "PLAY";

typedef struct SpriteCall {
    s32 x;
    s32 width;
    s32 u;
} SpriteCall;

static GameFrameContext s_frame;
static u8 s_packets[64];
static SpriteCall s_sprites[4];
static s32 s_spriteCount;
static s32 s_lineY[8];
static s32 s_lineCount;
static s32 s_tileCount;
static s32 s_modeCount;
static s32 s_leftEnabled;
static s32 s_rightEnabled;
static const char *s_text;

u8 *DrawLeftArrow(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                  s32 enabled) {
    (void)ot;
    (void)x;
    (void)y;
    s_leftEnabled = enabled;
    return prim + 1;
}

u8 *DrawRightArrow(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                   s32 enabled) {
    (void)ot;
    (void)x;
    (void)y;
    s_rightEnabled = enabled;
    return prim + 1;
}

u8 *GameQueueSpriteTrans(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                         s32 width, s32 height, s32 u, s32 v, s32 clut) {
    (void)ot;
    (void)y;
    (void)height;
    (void)v;
    (void)clut;
    s_sprites[s_spriteCount++] = (SpriteCall){x, width, u};
    return prim + 1;
}

u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *prim, s32 tpage) {
    (void)ot;
    (void)tpage;
    s_modeCount++;
    return prim + 1;
}

u8 *AddTilePrim(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y, s32 w,
                s32 h, s32 r, s32 g, s32 b) {
    (void)ot;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)r;
    (void)g;
    (void)b;
    s_tileCount++;
    return prim + 1;
}

u8 *GameQueueLine(GameOrderingTableEntry *ot, u8 *prim, s32 x0, s32 y0,
                  s32 x1, s32 y1, s32 r, s32 g, s32 b) {
    (void)ot;
    (void)x0;
    (void)x1;
    (void)y1;
    (void)r;
    (void)g;
    (void)b;
    s_lineY[s_lineCount++] = y0;
    return prim + 1;
}

void DrawSpriteString(s32 x, s32 y, const char *text, s32 clut) {
    (void)x;
    (void)y;
    (void)clut;
    s_text = text;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void Reset(void) {
    memset(&s_frame, 0, sizeof(s_frame));
    memset(s_sprites, 0, sizeof(s_sprites));
    memset(s_lineY, 0, sizeof(s_lineY));
    g_DrawBuffer = &s_frame;
    g_RenderState.packetCursor = s_packets;
    s_spriteCount = 0;
    s_lineCount = 0;
    s_tileCount = 0;
    s_modeCount = 0;
    s_leftEnabled = -1;
    s_rightEnabled = -1;
    s_text = NULL;
}

static int TestSteerPlayGauge(void) {
    Reset();
    g_NegconSteerPlay = 2;
    DrawNegconSteerPlayScreen();
    CHECK(s_text == g_MsgNegconSteerPlay);
    CHECK(s_leftEnabled == 1 && s_rightEnabled == 1);
    CHECK(s_spriteCount == 3 && s_modeCount == 1 && s_tileCount == 2);
    CHECK(s_lineCount == 6);
    CHECK(s_lineY[0] == 218 && s_lineY[1] == 219);
    CHECK(s_lineY[2] == 242 && s_lineY[3] == 243);
    CHECK(s_lineY[4] == 230 && s_lineY[5] == 231);
    CHECK(g_RenderState.packetCursor == s_packets + 14);
    return 0;
}

static int TestMaxTwistGauge(void) {
    Reset();
    g_NegconMaxTwist = NEGCON_CALIBRATION_FIRST;
    DrawNegconMaxTwistScreen();
    CHECK(s_text == g_MsgNegconMaxTwist);
    CHECK(s_leftEnabled == 0 && s_rightEnabled == 1);
    CHECK(s_sprites[0].x == 0x94 && s_sprites[0].width == 0x18);
    CHECK(s_sprites[0].u == 0);
    CHECK(g_RenderState.packetCursor == s_packets + 7);

    Reset();
    g_NegconMaxTwist = NEGCON_CALIBRATION_LAST;
    DrawNegconMaxTwistScreen();
    CHECK(s_leftEnabled == 1 && s_rightEnabled == 0);
    CHECK(s_sprites[0].x == 0x88 && s_sprites[0].width == 0x24);
    CHECK(s_sprites[0].u == 72);
    return 0;
}

int main(void) {
    CHECK(TestSteerPlayGauge() == 0);
    CHECK(TestMaxTwistGauge() == 0);
    puts("Negcon calibration drawing tests passed");
    return 0;
}
