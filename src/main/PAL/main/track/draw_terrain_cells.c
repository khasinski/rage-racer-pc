#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track_internal.h"
#include "game/terrain_internal.h"


static void DrawTerrainCellsFrom(s32 viewOffset) {
    BuildVisibleCells(viewOffset, 0x14000);
    SetRotMatrix(&g_RenderState.matrix);
    SubmitTerrainCells(&g_RenderState, g_VisibleCellList, 0x40);
}

void DrawTerrainCells(void) {
    DrawTerrainCellsFrom(-12288);
}

void DrawTerrainCellsWide(void) {
    DrawTerrainCellsFrom((s32)0xFFFF6000);
}

static s32 DivideSigned32(s32 value)
{
  if (value < 0)
    value += 31;
  return value >> 5;
}

static s32 GameRoundTerrainCoordinate(s32 value)
{
  return value / 256;
}

static s32 GameRoundTerrainCoordinate11(s32 value)
{
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

enum SkyOrderingTableIndex
{
  SKY_OT_FAR = 702,
  SKY_OT_NEAR = 703
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
    OT_TYPE *orderingTable;
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
    quad->x0 = GameRoundTerrainCoordinate11(leftX - band->sinRoll);
    quad->x1 = GameRoundTerrainCoordinate11(rightX - band->sinRoll);
    quad->x2 = GameRoundTerrainCoordinate11(leftX + band->sinRoll);
    quad->x3 = GameRoundTerrainCoordinate11(rightX + band->sinRoll);
    quad->y0 = GameRoundTerrainCoordinate11(topY - band->cosRoll);
    quad->y1 = GameRoundTerrainCoordinate11(bottomY - band->cosRoll);
    quad->y2 = GameRoundTerrainCoordinate11(topY + band->cosRoll);
    quad->y3 = GameRoundTerrainCoordinate11(bottomY + band->cosRoll);
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

    edge.x0 = GameRoundTerrainCoordinate(band->panelX + band->rowStepX * 3);
    edge.x1 = GameRoundTerrainCoordinate(
        band->panelX + band->columnStepX + band->rowStepX * 3);
    edge.y0 = GameRoundTerrainCoordinate(band->panelY + band->rowStepY * 3);
    edge.y1 = GameRoundTerrainCoordinate(
        band->panelY + band->columnStepY + band->rowStepY * 3);
    return edge;
}

static u8 *DrawNearGroundGradient(SkyFrame *frame,
                                  const SkyBandGeometry *band,
                                  const SkySkirtEdge *bottomEdge,
                                  u8 *packetCursor) {
    POLY_G4 *quad = (POLY_G4 *)packetCursor;

    SetPolyG4(quad);
    quad->x0 = GameRoundTerrainCoordinate(band->panelX + band->rowStepX);
    quad->x1 = GameRoundTerrainCoordinate(
        band->panelX + band->columnStepX + band->rowStepX);
    quad->x2 = bottomEdge->x0;
    quad->x3 = bottomEdge->x1;
    quad->y0 = GameRoundTerrainCoordinate(band->panelY + band->rowStepY);
    quad->y1 = GameRoundTerrainCoordinate(
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
    s32 savedSinRoll;
    s32 savedCosRoll;
    s32 textureColumn;
    s32 bandOriginXFixed;
    s32 bandOriginYFixed;
    s32 lowerPanelXFixed;
    s32 cellXFixed;
    s32 coordinateAccumulator;
} SkyBandSetup;

/*
 * Where the sky sits behind the camera this frame, and how far each row
 * and column of it steps across the screen. All of it follows from the
 * camera angles, so none of the drawing below needs them again.
 */
static void MeasureSkyBand(SkyFrame *work,
                           SkyBandSetup *band) {
    s32 cameraY;
    s32 bandRowY;
    s32 rotatedBandY;
    s32 sinRoll;
    s32 nearVerticalFixed;
    s32 horizontalFixed;
    s32 farVerticalFixed;
    s32 negativeSinRoll;
    s32 cosRoll;
    s32 angleWork;
    s32 leftViewAngle;
    s32 rollAngle;
    s32 rotatedX;
    s32 rotatedY;
    s32 pitchAngle;
    s32 yawAngle;
    s32 unroundedX;
    s32 unroundedY;
    if (work->mirrorFlag != g_MirrorMode)
    {
      pitchAngle = -work->pitch;
    }
    else
    {
      pitchAngle = work->pitch;
    }
    band->coordinateAccumulator = pitchAngle & 0xFFF;
    leftViewAngle = band->coordinateAccumulator;
    if (leftViewAngle >= 0x800)
    {
      leftViewAngle -= 0x1000;
    }
    if (band->coordinateAccumulator >= 0x800)
    {
      band->coordinateAccumulator -= 0x1000;
    }
    cameraY = work->cameraY;
    {
      s32 leftPlusTwo;
      s32 rightPlusTwo;
      angleWork = cameraY - 6000;
      leftPlusTwo = leftViewAngle + 2;
      band->cellXFixed = DivideSigned32(angleWork);
      leftViewAngle = leftPlusTwo + band->cellXFixed;
      rightPlusTwo = band->coordinateAccumulator + 2;
      band->coordinateAccumulator = rightPlusTwo + band->cellXFixed;
      yawAngle = work->mirrorFlag;
    }
    angleWork = work->yaw;
    if (yawAngle != 0)
    {
      angleWork = -angleWork;
      yawAngle = angleWork + 0x200;
    }
    else
    {
      yawAngle = angleWork + 0x200;
    }
    angleWork = yawAngle & 0xFFF;
    leftViewAngle /= 2;
    band->coordinateAccumulator = (band->coordinateAccumulator / 2) + 0x50;
    band->textureColumn = angleWork >> 7;
    horizontalFixed = ((u32)((-0x100) - ((angleWork >> 1) & 0x3F))) << 8;
    nearVerticalFixed = ((u32)((-0x80) - leftViewAngle)) << 8;
    rollAngle = work->roll;
    farVerticalFixed = ((u32)((-0x80) - band->coordinateAccumulator)) << 8;
    if (g_MirrorMode == 0)
    {
      rollAngle = -rollAngle;
    }
    sinRoll = rsin(rollAngle);
    cosRoll = rcos(rollAngle);
    band->coordinateAccumulator = cosRoll * horizontalFixed;
    rotatedX = band->coordinateAccumulator + (sinRoll * nearVerticalFixed);
    unroundedX = rotatedX;
    if (unroundedX < 0)
    {
      rotatedX += 0xFFF;
    }
    negativeSinRoll = -sinRoll;
    rotatedBandY = negativeSinRoll * horizontalFixed;
    rotatedX >>= 0xC;
    leftViewAngle = 0xA000;
    band->panelXFixed = rotatedX + leftViewAngle;
    rotatedY = rotatedBandY + (cosRoll * nearVerticalFixed);
    unroundedY = rotatedY;
    if (unroundedY < 0)
    {
      rotatedY += 0xFFF;
    }
    bandRowY = rotatedY >> 0xC;
    band->coordinateAccumulator += sinRoll * farVerticalFixed;
    band->panelYFixed = bandRowY + 0x7800;
    if (band->coordinateAccumulator < 0)
    {
      band->coordinateAccumulator += 0xFFF;
    }
    rotatedY = cosRoll * farVerticalFixed;
    rotatedBandY += rotatedY;
    rotatedY = band->coordinateAccumulator >> 0xC;
    band->lowerPanelXFixed = rotatedY + 0xA000;
    if (rotatedBandY < 0)
    {
      rotatedBandY += 0xFFF;
    }
    leftViewAngle = rotatedBandY >> 0xC;
    band->coordinateAccumulator = leftViewAngle + 0x7800;
    if (g_MirrorMode != g_RenderState.orderingFlag)
    {
      band->panelYFixed = 0x2400;
      band->panelYFixed = bandRowY + band->panelYFixed;
      band->coordinateAccumulator = leftViewAngle + 0x2400;
    }
    band->columnStepX = cosRoll * 4;
    band->columnStepY = negativeSinRoll * 4;
    band->savedSinRoll = sinRoll;
    band->savedCosRoll = cosRoll;
    band->bandOriginXFixed = band->panelXFixed;
    band->bandOriginYFixed = band->panelYFixed;
    band->rowStepX = sinRoll * 8;
    band->rowStepY = cosRoll * 8;
}

static void InitializeSkyFrame(SkyFrame *work) {
    work->packetCursor = RENDER_PRIM_CURSOR_AS(u8);
    work->orderingTable = RENDER_OT_BASE_AS(OT_TYPE);
    work->cameraY = g_RenderState.viewY;
    work->pitch = g_RenderState.viewAngleX;
    work->yaw = g_RenderState.viewAngleY;
    work->roll = g_RenderState.viewAngleZ;
    work->mirrorFlag = g_RenderState.orderingFlag;
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
                                             [(band->textureColumn + column) & 0xF];
            const SkyTileUV *tileUv = &g_SkyTileUV[tileIndex];
            s32 nextCellX = cellX + band->columnStepX;
            s32 nextCellY = cellY + band->columnStepY;
            s32 leftX = cellX - rowShearX;
            s32 rightX = nextCellX - rowShearX;
            s32 topY = cellY - rowShearY;
            s32 bottomY = nextCellY - rowShearY;
            GpuUvAddress uvAddress;

            SetPolyFT4(quad);
            SetShadeTex(quad, 0);
            quad->tpage = 0x18;
            uvAddress.bytes = &quad->u0;
            *uvAddress.packed = tileUv->corner[0].packed;
            uvAddress.bytes = &quad->u1;
            *uvAddress.packed = tileUv->corner[1].packed;
            uvAddress.bytes = &quad->u2;
            *uvAddress.packed = tileUv->corner[2].packed;
            uvAddress.bytes = &quad->u3;
            *uvAddress.packed = tileUv->corner[3].packed;
            screenX[0] = GameRoundTerrainCoordinate(leftX);
            screenX[1] = GameRoundTerrainCoordinate(rightX);
            screenX[2] = GameRoundTerrainCoordinate(leftX + band->rowStepX);
            screenX[3] = GameRoundTerrainCoordinate(rightX + band->rowStepX);
            quad->x0 = screenX[0];
            quad->x1 = screenX[1];
            quad->x2 = screenX[2];
            quad->x3 = screenX[3];
            quad->y0 = GameRoundTerrainCoordinate(topY);
            quad->y1 = GameRoundTerrainCoordinate(bottomY);
            quad->y2 = GameRoundTerrainCoordinate(topY + band->rowStepY);
            quad->y3 = GameRoundTerrainCoordinate(bottomY + band->rowStepY);
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
    s32 panelY = band->coordinateAccumulator;

    for (s32 column = 0; column < 8; column++) {
        s32 nextPanelX = panelX + band->columnStepX;
        s32 nextPanelY = panelY + band->columnStepY;
        s32 screenX[4] = {
            GameRoundTerrainCoordinate(panelX),
            GameRoundTerrainCoordinate(nextPanelX),
            GameRoundTerrainCoordinate(panelX + band->rowStepX),
            GameRoundTerrainCoordinate(nextPanelX + band->rowStepX),
        };

        if (SkyQuadIntersectsScreen(screenX)) {
            POLY_FT4 *quad = (POLY_FT4 *)packetCursor;
            s32 tileIndex = g_SkyTileMap[0]
                                            [(band->textureColumn + column) & 0xF];
            const SkyTileUV *tileUv = &g_SkyTileUV[tileIndex];
            GpuUvAddress uvAddress;

            SetPolyFT4(quad);
            SetShadeTex(quad, 0);
            quad->tpage = 0x18;
            uvAddress.bytes = &quad->u0;
            *uvAddress.packed = tileUv->corner[0].packed;
            uvAddress.bytes = &quad->u1;
            *uvAddress.packed = tileUv->corner[1].packed;
            uvAddress.bytes = &quad->u2;
            *uvAddress.packed = tileUv->corner[2].packed;
            uvAddress.bytes = &quad->u3;
            *uvAddress.packed = tileUv->corner[3].packed;
            quad->x0 = screenX[0];
            quad->x1 = screenX[1];
            quad->x2 = screenX[2];
            quad->x3 = screenX[3];
            quad->y0 = GameRoundTerrainCoordinate(panelY);
            quad->y1 = GameRoundTerrainCoordinate(nextPanelY);
            quad->y2 = GameRoundTerrainCoordinate(panelY + band->rowStepY);
            quad->y3 = GameRoundTerrainCoordinate(nextPanelY + band->rowStepY);
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
    const s32 panelX = band->bandOriginXFixed;
    const s32 panelY = band->bandOriginYFixed;
    const s32 columnStepX = band->columnStepX * 8;
    const s32 columnStepY = band->columnStepY * 8;
    const s32 bandRightX = panelX + columnStepX;
    const s32 bandBottomY = panelY + columnStepY;
    s32 screenX[4] = {
        GameRoundTerrainCoordinate(panelX),
        GameRoundTerrainCoordinate(bandRightX),
        GameRoundTerrainCoordinate(panelX + band->rowStepX),
        GameRoundTerrainCoordinate(bandRightX + band->rowStepX),
    };
    s32 screenY[4] = {
        GameRoundTerrainCoordinate(panelY),
        GameRoundTerrainCoordinate(bandBottomY),
        GameRoundTerrainCoordinate(panelY + band->rowStepY),
        GameRoundTerrainCoordinate(bandBottomY + band->rowStepY),
    };
    GameEnvColor darkSky = {.bytes = {0, 0, 16, 0}};

    packetCursor = DrawSkyGradientQuad(
        work, packetCursor, screenX, screenY,
        g_EnvironmentColors.fields.slots[ENV_SKY_MIDDLE].cur,
        g_EnvironmentColors.fields.slots[ENV_SKY_HORIZON].cur);

    screenX[2] = GameRoundTerrainCoordinate(panelX - band->rowStepX);
    screenX[3] = GameRoundTerrainCoordinate(bandRightX - band->rowStepX);
    screenY[2] = GameRoundTerrainCoordinate(panelY - band->rowStepY);
    screenY[3] = GameRoundTerrainCoordinate(bandBottomY - band->rowStepY);
    packetCursor = DrawSkyGradientQuad(
        work, packetCursor, screenX, screenY,
        g_EnvironmentColors.fields.slots[ENV_SKY_MIDDLE].cur,
        g_EnvironmentColors.fields.slots[ENV_SKY_TOP].cur);

    screenX[1] = screenX[3];
    screenX[2] = GameRoundTerrainCoordinate(panelX - band->rowStepX * 3);
    screenX[3] = GameRoundTerrainCoordinate(bandRightX - band->rowStepX * 3);
    screenY[0] = screenY[2];
    screenY[1] = screenY[3];
    screenY[2] = GameRoundTerrainCoordinate(panelY - band->rowStepY * 3);
    screenY[3] = GameRoundTerrainCoordinate(bandBottomY - band->rowStepY * 3);
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

    geometry.panelX = setup.bandOriginXFixed;
    geometry.panelY = setup.bandOriginYFixed;
    geometry.columnStepX = setup.columnStepX * 8;
    geometry.columnStepY = setup.columnStepY * 8;
    geometry.rowStepX = setup.rowStepX;
    geometry.rowStepY = setup.rowStepY;
    geometry.sinRoll = setup.savedSinRoll;
    geometry.cosRoll = setup.savedCosRoll;

    if (g_SkyRowBase != 0) {
        packetCursor = DrawTexturedSkyGrid(&work, &setup, packetCursor);
    } else {
        packetCursor = DrawHorizonTileStrip(&work, &setup, packetCursor);
        packetCursor = DrawSkyGradientBands(&work, &setup, packetCursor);
    }

    packetCursor = DrawCourseSkirt(&work, &geometry, packetCursor);
    RENDER_PRIM_CURSOR_AS(u8) = packetCursor;
}
