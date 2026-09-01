#include "game/prim.h"
#include "game/car.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"

DrawPacket *DrawMirrorFrame(u8 *packet) {
    OT_TYPE *otArg;
    u8 *prim;
    OT_TYPE *ot;
    s32 colorIndex;
    s32 paletteIndex;
    s32 color;
    u8 *next;
    TILE *tile;
    RenderBufferAddress tileAddress;

    ot = &GetGameFrameContext(g_DrawBuffer)->layout.orderingTables[0][1];

    tileAddress.bytes = packet;
    tile = tileAddress.tile;
    SetTile(tile);
    otArg = ot;
    prim = tileAddress.bytes;

    tile->x0 = 0x54;
    color = 0x98;
    tile->r0 = 0;
    tile->g0 = 0;
    tile->b0 = 0;
    tile->w = color;
    tile->y0 = (s16)(g_MirrorPanelY - 2);
    tile->h = 0x28;
    packet += sizeof(*tile);
    AddPrim(otArg, prim);

    colorIndex = g_CarMirrorBadgeStyles[g_PlayerCarIndex];
    paletteIndex = colorIndex * 3;
    ot = &GetGameFrameContext(g_DrawBuffer)->layout.orderingTables[1][1];
    next = GameQueueSprite(ot, packet, 0x56, g_MirrorPanelY, g_MirrorBadgeWidths[paletteIndex], 8, g_MirrorBadgeTexU[paletteIndex], g_MirrorBadgeTexV[paletteIndex], 0x7800);
    tileAddress.bytes = QueueDrawModePrim(ot, next, 9);
    return tileAddress.drawPacket;
}


void DrawRearViewMirror(s32 mode) {
    void **state;
    DrawPacket *packet;
    DrawPacket *prim;

    if (mode >= 0x169) {
        g_MirrorUnlocked = 1;
    }

    if (g_MirrorUnlocked != 0) {
        if (g_MirrorViewEnabled != 0) {
            if (g_MirrorPanelY < 0x12) {
                g_MirrorPanelY++;
            }
        } else if (g_MirrorPanelY >= -0x2B) {
            g_MirrorPanelY--;
        }

        if (BeginMirrorPass() != 0) {
            state = &RENDER_PRIM_CURSOR_AS(void);

            DrawSkyBackground();
            packet = DrawMirrorFrame(*state);
            SetDrawArea(packet,
                        &GetGameFrameContext(g_DrawBuffer)
                             ->environment.mirrorDraw.clip);
            prim = packet;
            packet++;
            AddPrim(&GetGameFrameContext(g_DrawBuffer)->layout.orderingTables[1]
                         [GAME_FRAME_OT_LENGTH - 1], prim);
            *state = packet;
            BuildVisibleCells(-0x3000, PortMirrorFarDepth(0x6000));
            SetRotMatrix((&g_RenderState.matrix));
            g_RenderState.envMode4 = g_IsEnvironmentMode4;
            SubmitTerrainCells((&g_RenderState), g_VisibleCellList, 0x40);

            packet = *state;
            SetDrawArea(packet,
                        &GetGameFrameContext(g_DrawBuffer)->environment.draw.clip);
            prim = packet;
            packet++;
            AddPrim(&GetGameFrameContext(g_DrawBuffer)->layout.orderingTables[1][1],
                    prim);
            *state = packet;
            DrawCourseObjects();
            DrawCars();
            EndMirrorPass();
        }
    }
}
