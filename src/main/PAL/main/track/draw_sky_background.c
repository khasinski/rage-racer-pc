#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track_internal.h"
#include "game/terrain_internal.h"

static s32 DivideBy32TowardZero(s32 value) {
    if (value < 0) {
        value += 31;
    }
    return value >> 5;
}

static s32 Fixed8ToScreen(s32 value) {
    return value / 256;
}

static s32 Fixed11ToScreen(s32 value) {
    return value / 2048;
}

static void SetSkyGradientColors(POLY_G4 *quad,
                                 GameEnvColor nearColor,
                                 GameEnvColor farColor) {
    quad->r0 = quad->r1 = nearColor.bytes.r;
    quad->g0 = quad->g1 = nearColor.bytes.g;
    quad->b0 = quad->b1 = nearColor.bytes.b;
    quad->r2 = quad->r3 = farColor.bytes.r;
    quad->g2 = quad->g3 = farColor.bytes.g;
    quad->b2 = quad->b3 = farColor.bytes.b;
}

enum SkyOrderingTableIndex {
    SKY_OT_FAR = 702,
    SKY_OT_NEAR = 703,
};
/*
 * The sky's geometry for this frame: where the bands start on screen, how far
 * a step along each axis moves, and the roll they were rotated by. The band
 * setup works it out once and the drawing reads it.
 */
typedef struct SkyBandGeometry {
    s32 panelX;
    s32 panelY;
    s32 columnStepX;
    s32 columnStepY;
    s32 rowStepX;
    s32 rowStepY;
    s32 sinRoll;
    s32 cosRoll;
} SkyBandGeometry;

typedef struct SkySkirtEdge {
    s32 x0;
    s32 x1;
    s32 y0;
    s32 y1;
} SkySkirtEdge;

typedef struct SkyFrame {
    u8 *packetCursor;
    GameOrderingTableEntry *orderingTable;
    s32 cameraY;
    s32 pitch;
    s32 yaw;
    s32 roll;
    s32 mirrorFlag;
} SkyFrame;

/*
 * The skirt under the sky: the band of quads that closes the gap between the
 * horizon and the terrain. Two courses want it drawn differently, which is the
 * only thing the course index decides here. Returns where it left the packet
 * cursor.
 */
static u8 *DrawFarGroundGradient(SkyFrame *frame,
                                 const SkyBandGeometry *band,
                                 SkySkirtEdge *bottomEdge,
                                 u8 *packetCursor) {
    POLY_G4 *quad = (POLY_G4 *)packetCursor;
    s32 leftX = (band->panelX + band->rowStepX) * 8;
    s32 rightX = (band->panelX + band->columnStepX + band->rowStepX) * 8;
    s32 topY = (band->panelY + band->rowStepY) * 8;
    s32 bottomY = (band->panelY + band->columnStepY + band->rowStepY) * 8;

    SetPolyG4(quad);
    quad->x0 = Fixed11ToScreen(leftX - band->sinRoll);
    quad->x1 = Fixed11ToScreen(rightX - band->sinRoll);
    quad->x2 = Fixed11ToScreen(leftX + band->sinRoll);
    quad->x3 = Fixed11ToScreen(rightX + band->sinRoll);
    quad->y0 = Fixed11ToScreen(topY - band->cosRoll);
    quad->y1 = Fixed11ToScreen(bottomY - band->cosRoll);
    quad->y2 = Fixed11ToScreen(topY + band->cosRoll);
    quad->y3 = Fixed11ToScreen(bottomY + band->cosRoll);
    SetSkyGradientColors(
        quad, g_EnvironmentColors.fields.slots[ENV_GROUND_FAR_TOP].cur,
        g_EnvironmentColors.fields.slots[ENV_GROUND_FAR_BOTTOM].cur);
    AddPrim(&frame->orderingTable[SKY_OT_FAR], quad);

    bottomEdge->x0 = quad->x2;
    bottomEdge->x1 = quad->x3;
    bottomEdge->y0 = quad->y2;
    bottomEdge->y1 = quad->y3;
    return packetCursor + sizeof(*quad);
}

static SkySkirtEdge MeasureLowerSkirtEdge(const SkyBandGeometry *band) {
    SkySkirtEdge edge;

    edge.x0 = Fixed8ToScreen(band->panelX + band->rowStepX * 3);
    edge.x1 = Fixed8ToScreen(
        band->panelX + band->columnStepX + band->rowStepX * 3);
    edge.y0 = Fixed8ToScreen(band->panelY + band->rowStepY * 3);
    edge.y1 = Fixed8ToScreen(
        band->panelY + band->columnStepY + band->rowStepY * 3);
    return edge;
}

static u8 *DrawNearGroundGradient(SkyFrame *frame,
                                  const SkyBandGeometry *band,
                                  const SkySkirtEdge *bottomEdge,
                                  u8 *packetCursor) {
    POLY_G4 *quad = (POLY_G4 *)packetCursor;

    SetPolyG4(quad);
    quad->x0 = Fixed8ToScreen(band->panelX + band->rowStepX);
    quad->x1 = Fixed8ToScreen(
        band->panelX + band->columnStepX + band->rowStepX);
    quad->x2 = bottomEdge->x0;
    quad->x3 = bottomEdge->x1;
    quad->y0 = Fixed8ToScreen(band->panelY + band->rowStepY);
    quad->y1 = Fixed8ToScreen(
        band->panelY + band->columnStepY + band->rowStepY);
    quad->y2 = bottomEdge->y0;
    quad->y3 = bottomEdge->y1;
    SetSkyGradientColors(
        quad, g_EnvironmentColors.fields.slots[ENV_GROUND_NEAR_TOP].cur,
        g_EnvironmentColors.fields.slots[ENV_GROUND_NEAR_BOTTOM].cur);
    AddPrim(&frame->orderingTable[SKY_OT_NEAR], quad);
    return packetCursor + sizeof(*quad);
}

static u8 *DrawFlatCourseSkirt(SkyFrame *frame,
                               const SkySkirtEdge *topEdge,
                               const SkySkirtEdge *bottomEdge,
                               u8 *packetCursor) {
    POLY_F4 *quad = (POLY_F4 *)packetCursor;
    GameEnvColor color =
        g_EnvironmentColors.fields.slots[ENV_SKY_BOTTOM].cur;

    SetPolyF4(quad);
    quad->x0 = topEdge->x0;
    quad->x1 = topEdge->x1;
    quad->x2 = bottomEdge->x0;
    quad->x3 = bottomEdge->x1;
    quad->y0 = topEdge->y0;
    quad->y1 = topEdge->y1;
    quad->y2 = bottomEdge->y0;
    quad->y3 = bottomEdge->y1;
    quad->r0 = color.bytes.r;
    quad->g0 = color.bytes.g;
    quad->b0 = color.bytes.b;
    AddPrim(&frame->orderingTable[SKY_OT_NEAR], quad);
    return packetCursor + sizeof(*quad);
}

static u8 *DrawCourseSkirt(SkyFrame *frame,
                           const SkyBandGeometry *band,
                           u8 *packetCursor) {
    SkySkirtEdge bottomEdge = MeasureLowerSkirtEdge(band);

    if (g_CourseIndex == 2) {
        return DrawNearGroundGradient(frame, band, &bottomEdge, packetCursor);
    }

    SkySkirtEdge topEdge;
    packetCursor =
        DrawFarGroundGradient(frame, band, &topEdge, packetCursor);
    return DrawFlatCourseSkirt(frame, &topEdge, &bottomEdge, packetCursor);
}

typedef struct SkyBandSetup {
    s32 panelXFixed;
    s32 panelYFixed;
    s32 columnStepX;
    s32 columnStepY;
    s32 rowStepX;
    s32 rowStepY;
    s32 sinRoll;
    s32 cosRoll;
    s32 textureColumn;
    s32 lowerPanelXFixed;
    s32 lowerPanelYFixed;
} SkyBandSetup;

/*
 * Where the sky sits behind the camera this frame, and how far each row
 * and column of it steps across the screen. All of it follows from the
 * camera angles, so none of the drawing below needs them again.
 */
static s32 SignedAngle12(s32 angle) {
    angle &= 0xFFF;
    return angle >= 0x800 ? angle - 0x1000 : angle;
}

static void MeasureSkyBand(const SkyFrame *frame, SkyBandSetup *band) {
    s32 pitch = frame->mirrorFlag != g_MirrorMode
                    ? -frame->pitch
                    : frame->pitch;
    s32 cameraPitch = SignedAngle12(pitch) + 2 +
                      DivideBy32TowardZero(frame->cameraY - 6000);
    s32 yaw = frame->mirrorFlag != 0 ? -frame->yaw : frame->yaw;
    s32 yawAngle = (yaw + 0x200) & 0xFFF;
    s32 nearVerticalFixed = (-0x80 - cameraPitch / 2) * 256;
    s32 farVerticalFixed = (-0x80 - (cameraPitch / 2 + 0x50)) * 256;
    s32 horizontalFixed = (-0x100 - ((yawAngle >> 1) & 0x3F)) * 256;
    s32 rollAngle = g_MirrorMode == 0 ? -frame->roll : frame->roll;
    s32 sinRoll = rsin(rollAngle);
    s32 cosRoll = rcos(rollAngle);
    s32 rotatedHorizontalY = -sinRoll * horizontalFixed;
    s32 nearX = cosRoll * horizontalFixed + sinRoll * nearVerticalFixed;
    s32 nearY = rotatedHorizontalY + cosRoll * nearVerticalFixed;
    s32 farX = cosRoll * horizontalFixed + sinRoll * farVerticalFixed;
    s32 farY = rotatedHorizontalY + cosRoll * farVerticalFixed;
    s32 verticalOrigin =
        g_MirrorMode != g_RenderState.orderingFlag ? 0x2400 : 0x7800;

    band->panelXFixed = nearX / 4096 + 0xA000;
    band->panelYFixed = nearY / 4096 + verticalOrigin;
    band->lowerPanelXFixed = farX / 4096 + 0xA000;
    band->lowerPanelYFixed = farY / 4096 + verticalOrigin;
    band->columnStepX = cosRoll * 4;
    band->columnStepY = -sinRoll * 4;
    band->sinRoll = sinRoll;
    band->cosRoll = cosRoll;
    band->rowStepX = sinRoll * 8;
    band->rowStepY = cosRoll * 8;
    band->textureColumn = yawAngle >> 7;
}

static void InitializeSkyFrame(SkyFrame *work) {
    work->packetCursor = RENDER_PRIM_CURSOR_AS(u8);
    work->orderingTable = RENDER_OT_BASE_AS(GameOrderingTableEntry);
    work->cameraY = g_RenderState.viewY;
    work->pitch = g_RenderState.viewAngleX;
    work->yaw = g_RenderState.viewAngleY;
    work->roll = g_RenderState.viewAngleZ;
    work->mirrorFlag = g_RenderState.orderingFlag;
}

static void SetSkyQuadUV(POLY_FT4 *quad, const SkyTileUV *tile) {
    quad->u0 = tile->corner[0].bytes.u;
    quad->v0 = tile->corner[0].bytes.v;
    quad->u1 = tile->corner[1].bytes.u;
    quad->v1 = tile->corner[1].bytes.v;
    quad->u2 = tile->corner[2].bytes.u;
    quad->v2 = tile->corner[2].bytes.v;
    quad->u3 = tile->corner[3].bytes.u;
    quad->v3 = tile->corner[3].bytes.v;
}

static u8 *DrawTexturedSkyGrid(SkyFrame *work,
                               const SkyBandSetup *band,
                               u8 *packetCursor) {
    s32 rowShearX = 0;
    s32 rowShearY = 0;
    s32 screenX[4];

    for (s32 row = 0; row < 4; row++) {
        s32 cellX = band->panelXFixed;
        s32 cellY = band->panelYFixed;

        for (s32 column = 0; column < 8; column++) {
            POLY_FT4 *quad = (POLY_FT4 *)packetCursor;
            s16 tileIndex = g_SkyTileMap[(row & 1) + g_SkyRowBase]
                [(band->textureColumn + column) &
                 (SKY_TILE_MAP_COLUMNS - 1)];
            const SkyTileUV *tileUv = &g_SkyTileUV[tileIndex];
            s32 nextCellX = cellX + band->columnStepX;
            s32 nextCellY = cellY + band->columnStepY;
            s32 leftX = cellX - rowShearX;
            s32 rightX = nextCellX - rowShearX;
            s32 topY = cellY - rowShearY;
            s32 bottomY = nextCellY - rowShearY;
            SetPolyFT4(quad);
            SetShadeTex(quad, 0);
            quad->tpage = 0x18;
            SetSkyQuadUV(quad, tileUv);
            screenX[0] = Fixed8ToScreen(leftX);
            screenX[1] = Fixed8ToScreen(rightX);
            screenX[2] = Fixed8ToScreen(leftX + band->rowStepX);
            screenX[3] = Fixed8ToScreen(rightX + band->rowStepX);
            quad->x0 = screenX[0];
            quad->x1 = screenX[1];
            quad->x2 = screenX[2];
            quad->x3 = screenX[3];
            quad->y0 = Fixed8ToScreen(topY);
            quad->y1 = Fixed8ToScreen(bottomY);
            quad->y2 = Fixed8ToScreen(topY + band->rowStepY);
            quad->y3 = Fixed8ToScreen(bottomY + band->rowStepY);
            quad->r0 = 0x80;
            quad->g0 = 0x80;
            quad->b0 = 0x80;
            quad->clut = 0x798E;
            AddPrim(&work->orderingTable[SKY_OT_NEAR], quad);

            packetCursor += sizeof(*quad);
            cellX = nextCellX;
            cellY = nextCellY;
        }

        rowShearX += band->rowStepX;
        rowShearY += band->rowStepY;
    }

    return packetCursor;
}

static s32 SkyQuadIntersectsScreen(const s32 screenX[4]) {
    s32 hasPointAtOrRightOfLeftEdge = 0;
    s32 hasPointLeftOfRightEdge = 0;

    for (s32 corner = 0; corner < 4; corner++) {
        hasPointAtOrRightOfLeftEdge |= screenX[corner] >= 0;
        hasPointLeftOfRightEdge |= screenX[corner] < 320;
    }

    return hasPointAtOrRightOfLeftEdge && hasPointLeftOfRightEdge;
}

static u8 *DrawHorizonTileStrip(SkyFrame *work,
                                const SkyBandSetup *band,
                                u8 *packetCursor) {
    s32 panelX = band->lowerPanelXFixed;
    s32 panelY = band->lowerPanelYFixed;

    for (s32 column = 0; column < 8; column++) {
        s32 nextPanelX = panelX + band->columnStepX;
        s32 nextPanelY = panelY + band->columnStepY;
        s32 screenX[4] = {
            Fixed8ToScreen(panelX),
            Fixed8ToScreen(nextPanelX),
            Fixed8ToScreen(panelX + band->rowStepX),
            Fixed8ToScreen(nextPanelX + band->rowStepX),
        };

        if (SkyQuadIntersectsScreen(screenX)) {
            POLY_FT4 *quad = (POLY_FT4 *)packetCursor;
            s32 tileIndex = g_SkyTileMap[0]
                [(band->textureColumn + column) &
                 (SKY_TILE_MAP_COLUMNS - 1)];
            const SkyTileUV *tileUv = &g_SkyTileUV[tileIndex];
            SetPolyFT4(quad);
            SetShadeTex(quad, 0);
            quad->tpage = 0x18;
            SetSkyQuadUV(quad, tileUv);
            quad->x0 = screenX[0];
            quad->x1 = screenX[1];
            quad->x2 = screenX[2];
            quad->x3 = screenX[3];
            quad->y0 = Fixed8ToScreen(panelY);
            quad->y1 = Fixed8ToScreen(nextPanelY);
            quad->y2 = Fixed8ToScreen(panelY + band->rowStepY);
            quad->y3 = Fixed8ToScreen(nextPanelY + band->rowStepY);
            quad->r0 = 0x80;
            quad->g0 = 0x80;
            quad->b0 = 0x80;
            quad->clut = 0x798E;
            AddPrim(&work->orderingTable[SKY_OT_NEAR], quad);
            packetCursor += sizeof(*quad);
        }

        panelX = nextPanelX;
        panelY = nextPanelY;
    }

    return packetCursor;
}

static u8 *DrawSkyGradientQuad(SkyFrame *work,
                               u8 *packetCursor,
                               const s32 screenX[4],
                               const s32 screenY[4],
                               GameEnvColor nearColor,
                               GameEnvColor farColor) {
    POLY_G4 *quad = (POLY_G4 *)packetCursor;

    SetPolyG4(quad);
    quad->x0 = screenX[0];
    quad->x1 = screenX[1];
    quad->x2 = screenX[2];
    quad->x3 = screenX[3];
    quad->y0 = screenY[0];
    quad->y1 = screenY[1];
    quad->y2 = screenY[2];
    quad->y3 = screenY[3];
    SetSkyGradientColors(quad, nearColor, farColor);
    AddPrim(&work->orderingTable[SKY_OT_NEAR], quad);
    return packetCursor + sizeof(*quad);
}

static u8 *DrawSkyGradientBands(SkyFrame *work,
                                const SkyBandSetup *band,
                                u8 *packetCursor) {
    const s32 panelX = band->panelXFixed;
    const s32 panelY = band->panelYFixed;
    const s32 columnStepX = band->columnStepX * 8;
    const s32 columnStepY = band->columnStepY * 8;
    const s32 bandRightX = panelX + columnStepX;
    const s32 bandBottomY = panelY + columnStepY;
    s32 screenX[4] = {
        Fixed8ToScreen(panelX),
        Fixed8ToScreen(bandRightX),
        Fixed8ToScreen(panelX + band->rowStepX),
        Fixed8ToScreen(bandRightX + band->rowStepX),
    };
    s32 screenY[4] = {
        Fixed8ToScreen(panelY),
        Fixed8ToScreen(bandBottomY),
        Fixed8ToScreen(panelY + band->rowStepY),
        Fixed8ToScreen(bandBottomY + band->rowStepY),
    };
    GameEnvColor darkSky = {.bytes = {0, 0, 16, 0}};

    packetCursor = DrawSkyGradientQuad(
        work, packetCursor, screenX, screenY,
        g_EnvironmentColors.fields.slots[ENV_SKY_MIDDLE].cur,
        g_EnvironmentColors.fields.slots[ENV_SKY_HORIZON].cur);

    screenX[2] = Fixed8ToScreen(panelX - band->rowStepX);
    screenX[3] = Fixed8ToScreen(bandRightX - band->rowStepX);
    screenY[2] = Fixed8ToScreen(panelY - band->rowStepY);
    screenY[3] = Fixed8ToScreen(bandBottomY - band->rowStepY);
    packetCursor = DrawSkyGradientQuad(
        work, packetCursor, screenX, screenY,
        g_EnvironmentColors.fields.slots[ENV_SKY_MIDDLE].cur,
        g_EnvironmentColors.fields.slots[ENV_SKY_TOP].cur);

    screenX[1] = screenX[3];
    screenX[2] = Fixed8ToScreen(panelX - band->rowStepX * 3);
    screenX[3] = Fixed8ToScreen(bandRightX - band->rowStepX * 3);
    screenY[0] = screenY[2];
    screenY[1] = screenY[3];
    screenY[2] = Fixed8ToScreen(panelY - band->rowStepY * 3);
    screenY[3] = Fixed8ToScreen(bandBottomY - band->rowStepY * 3);
    packetCursor = DrawSkyGradientQuad(
        work, packetCursor, screenX, screenY,
        g_EnvironmentColors.fields.slots[ENV_SKY_TOP].cur, darkSky);

    return packetCursor;
}

void DrawSkyBackground(void) {
    SkyFrame work;
    SkyBandSetup setup;
    SkyBandGeometry geometry;
    u8 *packetCursor;

    InitializeSkyFrame(&work);
    MeasureSkyBand(&work, &setup);
    packetCursor = work.packetCursor;

    geometry.panelX = setup.panelXFixed;
    geometry.panelY = setup.panelYFixed;
    geometry.columnStepX = setup.columnStepX * 8;
    geometry.columnStepY = setup.columnStepY * 8;
    geometry.rowStepX = setup.rowStepX;
    geometry.rowStepY = setup.rowStepY;
    geometry.sinRoll = setup.sinRoll;
    geometry.cosRoll = setup.cosRoll;

    if (g_SkyRowBase != 0) {
        packetCursor = DrawTexturedSkyGrid(&work, &setup, packetCursor);
    } else {
        packetCursor = DrawHorizonTileStrip(&work, &setup, packetCursor);
        packetCursor = DrawSkyGradientBands(&work, &setup, packetCursor);
    }

    packetCursor = DrawCourseSkirt(&work, &geometry, packetCursor);
    RENDER_PRIM_CURSOR_AS(u8) = packetCursor;
}
