#include "game/prim.h"
#include "game/car.h"
#include "game/car_render_rules.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track.h"

enum {
    MIRROR_FRAME_X = 0x54,
    MIRROR_FRAME_WIDTH = 0x98,
    MIRROR_FRAME_HEIGHT = 0x28,
    MIRROR_CONTENT_X = 0x56,
};

DrawPacket *DrawMirrorFrame(u8 *packet) {
    GameOrderingTableEntry *frameOt =
        &g_DrawBuffer->layout.orderingTables[0][1];
    GameOrderingTableEntry *contentOt =
        &g_DrawBuffer->layout.orderingTables[1][1];
    s32 badgeSpriteIndex;
    u8 *next;
    TILE *tile = (TILE *)packet;

    SetTile(tile);
    tile->x0 = MIRROR_FRAME_X;
    tile->r0 = 0;
    tile->g0 = 0;
    tile->b0 = 0;
    tile->w = MIRROR_FRAME_WIDTH;
    tile->y0 = (s16)(g_MirrorPanelY - 2);
    tile->h = MIRROR_FRAME_HEIGHT;
    AddPrim(frameOt, tile);
    packet = (u8 *)(tile + 1);

    badgeSpriteIndex = ResolveMirrorBadgeSpriteIndex(
        g_PlayerCarIndex, g_CarMirrorBadgeStyles, GAME_CAR_COUNT);
    next = GameQueueSprite(
        contentOt, packet, MIRROR_CONTENT_X, g_MirrorPanelY,
        g_MirrorBadgeWidths[badgeSpriteIndex], 8,
        g_MirrorBadgeTexU[badgeSpriteIndex],
        g_MirrorBadgeTexV[badgeSpriteIndex], 0x7800);
    return (DrawPacket *)QueueDrawModePrim(contentOt, next, 9);
}


static void UpdateMirrorPanelPosition(void) {
    if (g_MirrorViewEnabled == 0) {
        if (g_MirrorPanelY >= -0x2B) {
            g_MirrorPanelY--;
        }
        return;
    }

    if (g_MirrorPanelY < 0x12) {
        g_MirrorPanelY++;
    }
}

void DrawRearViewMirror(s32 mode) {
    DrawPacket *packet;

    if (mode >= 0x169) {
        g_MirrorUnlocked = 1;
    }

    if (g_MirrorUnlocked == 0) {
        return;
    }

    UpdateMirrorPanelPosition();
    if (BeginMirrorPass() == 0) {
        return;
    }

    DrawSkyBackground();
    packet = DrawMirrorFrame(g_RenderState.packetCursor);
    SetDrawArea(packet, &g_DrawBuffer->environment.mirrorDraw.clip);
    AddPrim(&g_DrawBuffer->layout.orderingTables[1][GAME_FRAME_OT_LENGTH - 1],
            packet);
    g_RenderState.packetCursor = packet + 1;
    BuildVisibleCells(-0x3000, PortMirrorFarDepth(0x6000));
    SetRotMatrix(&g_RenderState.matrix);
    g_RenderState.envMode4 = g_IsEnvironmentMode4;
    SubmitTerrainCells(&g_RenderState, g_VisibleCellList, 0x40);

    packet = g_RenderState.packetCursor;
    SetDrawArea(packet, &g_DrawBuffer->environment.draw.clip);
    AddPrim(&g_DrawBuffer->layout.orderingTables[1][1], packet);
    g_RenderState.packetCursor = packet + 1;
    DrawCourseObjects();
    DrawCars();
    EndMirrorPass();
}
