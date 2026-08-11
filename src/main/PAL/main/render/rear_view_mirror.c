#include "common.h"
#include "game/prim.h"
#include "game/car.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/scratchpad.h"
#include "psyq/gpu.h"
#include "psyq/gte.h"

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
 * Sets up the scratchpad render state (0x1F800000) for the rear-view mirror
 * pass: only when all five conditions hold (mirror flag, enabled, etc.) does it
 * save the current matrix into g_CameraMatrixSaved, install the mirror matrix g_MirrorViewMatrix,
 * set mode 9 + a narrow clip rect + prim base, flip the ordering flag, and push
 * the pass behind the main scene (depth += 0x800). Returns 1 if the mirror pass
 * is active, else 0.
 */
s32 BeginMirrorPass(void) {
    GameScratchpadRenderState *scratch;
    s32 mirrorEnabled;
    register s32 v0reg asm("$2");
    s32 v1reg;
    s32 y0;
    MirrorPanelPositionAddress panelPosition;

    mirrorEnabled = 0;
    scratch = SCRATCHPAD;

    if ((g_MirrorUnlocked != 0) &&
        (g_MirrorViewEnabled != 0) &&
        (g_CameraViewMode == CAMERA_VIEW_CAR) &&
        (g_GrandPrixMode != 0) &&
        (g_RacePhase == 2)) {
        mirrorEnabled = 1;
    }

    if (mirrorEnabled != 0) {
        g_CameraMatrixSaved = scratch->matrix;
        scratch->matrix = g_MirrorViewMatrix;

        SetGeomOffset(0xA0, 0x24);
        SetGeomScreen(0xC0);

        v0reg = 9;
        scratch->mode = v0reg;
        v0reg = 0x56;
        scratch->x0 = v0reg;
        panelPosition.position = &g_MirrorPanelY;
        y0 = *panelPosition.screenY;
        scratch->x1 = 0xEA;
        __asm__("");
        v1reg = g_MirrorPanelY;
        scratch->primData =
            &GetGameFrameContext(g_DrawBuffer)->layout.orderingTables[1][0];
        v0reg = scratch->orderingFlag;
        scratch->y0 = y0;
        v0reg ^= 1;
        scratch->orderingFlag = v0reg;
        v0reg = y0 + 0x24;
        scratch->y1 = v0reg;

        if (v1reg > 0) {
            g_MirrorDrawEnv0ClipY = y0;
            v0reg = y0 + 0xF0;
        } else {
            v0reg = 0xF0;
            g_MirrorDrawEnv0ClipY = 0;
        }
        g_MirrorDrawEnv1ClipY = v0reg;

        v0reg = g_MirrorPanelY;
        v1reg = v0reg + 0x24;
        if (v1reg > 0) {
            v0reg = v1reg - g_MirrorDrawEnv0ClipY;
            g_MirrorDrawEnv0ClipH = v0reg;
            g_MirrorDrawEnv1ClipH = v0reg;
        } else {
            g_MirrorDrawEnv0ClipH = 0;
            g_MirrorDrawEnv1ClipH = 0;
        }

        g_VisibleCellMask = g_MirrorVisibleCellMask;
        g_VisibleCellList = g_MirrorVisibleCellList;
        scratch->depth += 0x800;
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
    GameScratchpadRenderState *scratch;
    register s32 v0reg asm("$2");
    s32 v1reg;

    scratch = SCRATCHPAD;

    SetGeomOffset(0xA0, 0x78);
    SetGeomScreen(0x140);

    v0reg = 0xA;
    scratch->mode = v0reg;
    v0reg = 0x140;
    scratch->x1 = v0reg;
    v0reg = 0xF0;
    scratch->y1 = v0reg;
    g_VisibleCellMask = g_MainVisibleCellMask;
    g_VisibleCellList = g_MainVisibleCellList;
    v1reg = scratch->depth;
    scratch->x0 = 0;
    scratch->y0 = 0;
    scratch->primData =
        &GetGameFrameContext(g_DrawBuffer)->layout.orderingTables[0][0];
    v0reg = scratch->orderingFlag;
    scratch->depth = v1reg - 0x800;
    scratch->orderingFlag = v0reg ^ 1;
    scratch->matrix = g_CameraMatrixSaved;
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
    void **scratch;
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
            scratch = &SCRATCH_PRIM_CURSOR_AS(void);

            DrawSkyBackground();
            packet = DrawMirrorFrame(*scratch);
            SetDrawArea(packet,
                        &GetGameFrameContext(g_DrawBuffer)
                             ->environment.mirrorDraw.clip);
            prim = packet;
            packet++;
            AddPrim(&GetGameFrameContext(g_DrawBuffer)->layout.orderingTables[1]
                         [GAME_FRAME_OT_LENGTH - 1], prim);
            *scratch = packet;
            BuildVisibleCells(-0x3000, 0x6000);
            SetRotMatrix(SCRATCH_VIEW_MATRIX_GTE);
            SCRATCH_ENV_MODE4 = g_IsEnvironmentMode4;
            SubmitTerrainCells(SCRATCHPAD, g_VisibleCellList, 0x40);

            packet = *scratch;
            SetDrawArea(packet,
                        &GetGameFrameContext(g_DrawBuffer)->environment.draw.clip);
            prim = packet;
            packet++;
            AddPrim(&GetGameFrameContext(g_DrawBuffer)->layout.orderingTables[1][1],
                    prim);
            *scratch = packet;
            DrawCourseObjects();
            DrawCars();
            EndMirrorPass();
        }
    }
}
