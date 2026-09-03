#include "common.h"
#include "game/car.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"

#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;
GameFrameContext g_FrameContexts[2];
GameFrameContext *g_DrawBuffer;
s32 g_PlayerCarIndex;
u8 g_CarMirrorBadgeStyles[GAME_CAR_COUNT];
u8 g_MirrorBadgeTexU[1];
u8 g_MirrorBadgeTexV[1];
u8 g_MirrorBadgeWidths[1];
s32 g_MirrorPanelY;
s32 g_MirrorUnlocked;
s16 g_MirrorViewEnabled;
s32 g_IsEnvironmentMode4;

static u8 s_packets[512];
static s32 s_beginResult;
static s32 s_beginCalls;
static s32 s_endCalls;
static s32 s_skyCalls;
static s32 s_terrainCalls;
static s32 s_objectCalls;
static s32 s_carCalls;
static s32 s_terrainNear;
static s32 s_terrainFar;
static s32 s_lastWorldCall;
static s32 s_endOrder;

s32 ResolveMirrorBadgeSpriteIndex(s32 carIndex, const u8 *styles,
                                  s32 carCount) {
    (void)carIndex;
    (void)styles;
    (void)carCount;
    return 0;
}

s32 AdvanceMirrorPanelY(s32 currentY, int enabled) {
    return currentY + (enabled ? 2 : -2);
}

s32 BeginMirrorPass(void) {
    s_beginCalls++;
    return s_beginResult;
}

void EndMirrorPass(void) {
    s_endCalls++;
    s_endOrder = ++s_lastWorldCall;
}

void DrawSkyBackground(void) { s_skyCalls++; }

void DrawTerrainCellsInRange(s32 nearDepth, s32 farDepth) {
    s_terrainCalls++;
    s_terrainNear = nearDepth;
    s_terrainFar = farDepth;
    ++s_lastWorldCall;
}

void DrawCourseObjects(void) {
    s_objectCalls++;
    ++s_lastWorldCall;
}

void DrawCars(void) {
    s_carCalls++;
    ++s_lastWorldCall;
}

int PortMirrorFarDepth(int retailFar) { return retailFar / 2; }

void SetTile(TILE *prim) { memset(prim, 0, sizeof(*prim)); }
void AddPrim(void *ot, void *prim) {
    (void)ot;
    (void)prim;
}
#undef SetDrawArea
void SetDrawArea(DR_AREA *prim, RECT *rect) {
    (void)prim;
    (void)rect;
}

u8 *GameQueueSprite(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                    s32 w, s32 h, s32 u, s32 v, s32 clutIndex) {
    (void)ot;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)u;
    (void)v;
    (void)clutIndex;
    return prim + sizeof(SPRT);
}

u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *prim, s32 tpage) {
    (void)ot;
    (void)tpage;
    return prim + sizeof(DrawPacket);
}

static void Reset(void) {
    memset(g_FrameContexts, 0, sizeof(g_FrameContexts));
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    memset(s_packets, 0, sizeof(s_packets));
    g_DrawBuffer = &g_FrameContexts[0];
    g_RenderState.packetCursor = s_packets;
    g_MirrorPanelY = -44;
    g_MirrorUnlocked = 0;
    g_MirrorViewEnabled = 1;
    g_IsEnvironmentMode4 = 1;
    s_beginResult = 1;
    s_beginCalls = 0;
    s_endCalls = 0;
    s_skyCalls = 0;
    s_terrainCalls = 0;
    s_objectCalls = 0;
    s_carCalls = 0;
    s_lastWorldCall = 0;
    s_endOrder = 0;
}

int main(void) {
    Reset();
    DrawRearViewMirror(360);
    if (s_beginCalls != 0 || g_MirrorPanelY != -44) {
        puts("FAIL: locked mirror started rendering");
        return 1;
    }

    DrawRearViewMirror(361);
    if (g_MirrorUnlocked != 1 || g_MirrorPanelY != -42 ||
        s_beginCalls != 1 || s_skyCalls != 1 || s_terrainCalls != 1 ||
        s_objectCalls != 1 || s_carCalls != 1 || s_endCalls != 1 ||
        s_terrainNear != -0x3000 || s_terrainFar != 0x3000 ||
        g_RenderState.envMode4 != 1 || s_endOrder != 4) {
        puts("FAIL: mirror did not draw and restore its complete world pass");
        return 1;
    }

    Reset();
    g_MirrorUnlocked = 1;
    s_beginResult = 0;
    DrawRearViewMirror(0);
    if (s_beginCalls != 1 || s_skyCalls != 0 || s_terrainCalls != 0 ||
        s_endCalls != 0) {
        puts("FAIL: unavailable mirror pass emitted world geometry");
        return 1;
    }

    puts("rear-view mirror orchestrates its complete world pass");
    return 0;
}
