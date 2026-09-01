#include "game/prim.h"
#include "game/car.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"

typedef union MirrorPanelPositionAddress {
    s32 *position;
    u16 *screenY;
} MirrorPanelPositionAddress;

void ResetMirrorState(void) {
    g_MirrorViewEnabled = 1;
    g_MirrorPanelY = -0x2C;
    g_MirrorUnlocked = 0;
}


/*
 * Sets up the render state for the rear-view mirror
 * pass: only when all five conditions hold (mirror flag, enabled, etc.) does it
 * save the current matrix into g_CameraMatrixSaved, install the mirror matrix g_MirrorViewMatrix,
 * set mode 9 + a narrow clip rect + prim base, flip the ordering flag, and push
 * the pass behind the main scene (depth += 0x800). Returns 1 if the mirror pass
 * is active, else 0.
 */
s32 BeginMirrorPass(void) {
    GameRenderState *state;
    s32 mirrorEnabled;
    s32 v0reg;
    s32 v1reg;
    s32 y0;
    MirrorPanelPositionAddress panelPosition;

    mirrorEnabled = 0;
    state = (&g_RenderState);

    if ((g_MirrorUnlocked != 0) &&
        (g_MirrorViewEnabled != 0) &&
        (g_CameraViewMode == CAMERA_VIEW_CAR) &&
        (g_GrandPrixMode != 0) &&
        (g_RacePhase == 2)) {
        mirrorEnabled = 1;
    }

    if (mirrorEnabled != 0) {
        g_CameraMatrixSaved = state->matrix;
        state->matrix = g_MirrorViewMatrix;

        SetGeomOffset(0xA0, 0x24);
        SetGeomScreen(0xC0);

        v0reg = 9;
        state->mode = v0reg;
        /* Retail state+0x6c is one aliased word: the mirror mode write is
         * also the terrain OTZ/LOD shift read by SubmitTerrainCells.  The
         * native state representation keeps the meanings separate, so
         * reproduce the alias explicitly. */
        state->faceOtShift = v0reg;
        v0reg = 0x56;
        state->x0 = v0reg;
        panelPosition.position = &g_MirrorPanelY;
        y0 = *panelPosition.screenY;
        state->x1 = 0xEA;
        
        v1reg = g_MirrorPanelY;
        state->primData =
            &GetGameFrameContext(g_DrawBuffer)->layout.orderingTables[1][0];
        v0reg = state->orderingFlag;
        state->y0 = y0;
        v0reg ^= 1;
        state->orderingFlag = v0reg;
        v0reg = y0 + 0x24;
        state->y1 = v0reg;

        if (v1reg > 0) {
            g_FrameContexts[0].environment.mirrorDraw.clip.y = y0;
            v0reg = y0 + 0xF0;
        } else {
            v0reg = 0xF0;
            g_FrameContexts[0].environment.mirrorDraw.clip.y = 0;
        }
        g_FrameContexts[1].environment.mirrorDraw.clip.y = v0reg;

        v0reg = g_MirrorPanelY;
        v1reg = v0reg + 0x24;
        if (v1reg > 0) {
            v0reg = v1reg -
                g_FrameContexts[0].environment.mirrorDraw.clip.y;
            g_FrameContexts[0].environment.mirrorDraw.clip.h = v0reg;
            g_FrameContexts[1].environment.mirrorDraw.clip.h = v0reg;
        } else {
            g_FrameContexts[0].environment.mirrorDraw.clip.h = 0;
            g_FrameContexts[1].environment.mirrorDraw.clip.h = 0;
        }

        g_VisibleCellMask = g_MirrorVisibleCellMask;
        g_VisibleCellList = g_MirrorVisibleCellList;
        state->depth += 0x800;
    }

    return mirrorEnabled;
}

/*
 * Sibling of BeginMirrorPass: closes the mirror pass and restores the full-screen
 * main viewport render state (mode 0xA, full 0x140x0xF0 clip rect, prim base),
 * flips the ordering flag back, pulls the depth back (-= 0x800) and restores the
 * saved main-view matrix from g_CameraMatrixSaved.
 */
void EndMirrorPass(void) {
    GameRenderState *state;
    s32 v0reg;
    s32 v1reg;

    state = (&g_RenderState);

    SetGeomOffset(0xA0, 0x78);
    SetGeomScreen(0x140);

    v0reg = 0xA;
    state->mode = v0reg;
    state->faceOtShift = v0reg;
    v0reg = 0x140;
    state->x1 = v0reg;
    v0reg = 0xF0;
    state->y1 = v0reg;
    g_VisibleCellMask = g_MainVisibleCellMask;
    g_VisibleCellList = g_MainVisibleCellList;
    v1reg = state->depth;
    state->x0 = 0;
    state->y0 = 0;
    state->primData =
        &GetGameFrameContext(g_DrawBuffer)->layout.orderingTables[0][0];
    v0reg = state->orderingFlag;
    state->depth = v1reg - 0x800;
    state->orderingFlag = v0reg ^ 1;
    state->matrix = g_CameraMatrixSaved;
}

DrawPacket *DrawMirrorFrame(u8 *packet) {
    MirrorPanelPositionAddress panelPosition;
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
    panelPosition.position = &g_MirrorPanelY;
    tile->y0 = *panelPosition.screenY - 2;
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
