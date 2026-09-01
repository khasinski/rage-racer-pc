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
    s32 textureColumn;
    s32 screenX0;
    s32 screenX1;
    s32 screenX2;
    s32 screenX3;
} SkyBandGeometry;

/*
 * The skirt under the sky: the band of quads that closes the gap between the
 * horizon and the terrain. Two courses want it drawn differently, which is the
 * only thing the course index decides here. Returns where it left the packet
 * cursor.
 */
static u8 *DrawCourseSkirt(SkyRenderWork *work, SkyBandGeometry *band,
                           u8 *packetCursor) {
    /* Carried from the wide half of the skirt to the near half. Both are
     * drawn on the same condition, for every course but the third, so the
     * near half never reads what the wide half did not write. */
    s32 courseX0;
    s32 courseX1;
    s32 courseY1;
    s32 xWorkLate;
  s32 leftXWorkFixed;
  s32 rightXWorkFixed;
  s32 screenY0;
  s32 screenY1;
  s32 screenY2;
  s32 screenY3;
  u8 *nextPacket;
  RenderBufferAddress packetAddress;
      s32 courseTopY;
      s32 skirtBottomY;
      s32 skirtRightX;
      s32 courseBottomY;
      s32 skirtStepX;
      s32 skirtStepY;
      s32 courseLeftX;
      band->textureColumn = band->rowStepX * 4;
      if (g_CourseIndex != 2)
      {
        POLY_G4 *courseG4;
        RenderBufferAddress cursor;
        cursor.bytes = packetCursor;
        courseG4 = cursor.polyG4;
        leftXWorkFixed = (band->panelX + band->rowStepX) * 8;
        courseLeftX = leftXWorkFixed - band->sinRoll;
        band->screenX0 = courseLeftX / 2048;
        rightXWorkFixed = ((band->panelX + band->columnStepX) + band->rowStepX) * 8;
        band->screenX1 = GameRoundTerrainCoordinate11(rightXWorkFixed - band->sinRoll);
        band->screenX2 = GameRoundTerrainCoordinate11(leftXWorkFixed + band->sinRoll);
        courseX0 = band->screenX2;
        band->screenX3 = GameRoundTerrainCoordinate11(rightXWorkFixed + band->sinRoll);
        courseTopY = (band->panelY + band->rowStepY) * 8;
        courseX1 = band->screenX3;
        screenY0 = GameRoundTerrainCoordinate11(courseTopY - band->cosRoll);
        courseBottomY = ((band->panelY + band->columnStepY) + band->rowStepY) * 8;
        screenY1 = GameRoundTerrainCoordinate11(courseBottomY - band->cosRoll);
        screenY2 = GameRoundTerrainCoordinate11(courseTopY + band->cosRoll);
        xWorkLate = screenY2;
        screenY3 = GameRoundTerrainCoordinate11(courseBottomY + band->cosRoll);
        SetPolyG4(courseG4);
        courseG4->x0 = band->screenX0;
        courseG4->x1 = band->screenX1;
        courseG4->x2 = band->screenX2;
        courseG4->x3 = band->screenX3;
        courseG4->y0 = screenY0;
        courseG4->y1 = screenY1;
        courseG4->y2 = screenY2;
        courseG4->y3 = screenY3;
        courseY1 = screenY3;
        cursor.polyG4 = courseG4 + 1;
        nextPacket = cursor.bytes;
        SetSkyGradientColors(
            courseG4,
            g_EnvironmentColors.fields.slots[ENV_GROUND_FAR_TOP].cur,
            g_EnvironmentColors.fields.slots[ENV_GROUND_FAR_BOTTOM].cur);
        AddPrim(&work->orderingTable[SKY_OT_FAR], courseG4);
        packetCursor = nextPacket;
      }
      skirtStepX = band->rowStepX * 3;
      band->screenX2 = GameRoundTerrainCoordinate(band->panelX + skirtStepX);
      skirtRightX = band->panelX + band->columnStepX;
      band->screenX3 = GameRoundTerrainCoordinate(skirtRightX + skirtStepX);
      skirtStepY = band->rowStepY * 3;
      screenY2 = GameRoundTerrainCoordinate(band->panelY + skirtStepY);
      skirtBottomY = band->panelY + band->columnStepY;
      screenY3 = GameRoundTerrainCoordinate(skirtBottomY + skirtStepY);
      if (g_CourseIndex == 2)
      {
        POLY_G4 *courseG4;
        RenderBufferAddress cursor;
        cursor.bytes = packetCursor;
        courseG4 = cursor.polyG4;
        rightXWorkFixed = band->panelX;
        band->screenX0 = GameRoundTerrainCoordinate(rightXWorkFixed + band->rowStepX);
        band->screenX1 =
            GameRoundTerrainCoordinate(skirtRightX + band->rowStepX);
        screenY0 = GameRoundTerrainCoordinate(band->panelY + band->rowStepY);
        screenY1 = GameRoundTerrainCoordinate(skirtBottomY + band->rowStepY);
        cursor.polyG4 = courseG4 + 1;
        nextPacket = cursor.bytes;
        SetPolyG4(courseG4);
        courseG4->x0 = band->screenX0;
        courseG4->x1 = band->screenX1;
        courseG4->x2 = band->screenX2;
        courseG4->x3 = band->screenX3;
        courseG4->y0 = screenY0;
        courseG4->y1 = screenY1;
        courseG4->y2 = screenY2;
        courseG4->y3 = screenY3;
        SetSkyGradientColors(
            courseG4,
            g_EnvironmentColors.fields.slots[ENV_GROUND_NEAR_TOP].cur,
            g_EnvironmentColors.fields.slots[ENV_GROUND_NEAR_BOTTOM].cur);
        AddPrim(&work->orderingTable[SKY_OT_NEAR], courseG4);
        packetCursor = nextPacket;
      }
      else
      {
        POLY_F4 *courseF4;
        packetAddress.bytes = packetCursor;
        courseF4 = packetAddress.polyF4;
        band->screenX0 = courseX0;
        band->screenX1 = courseX1;
        screenY0 = xWorkLate;
        screenY1 = courseY1;
        SetPolyF4(courseF4);
        courseF4->x0 = band->screenX0;
        courseF4->x1 = band->screenX1;
        courseF4->x2 = band->screenX2;
        courseF4->x3 = band->screenX3;
        courseF4->y0 = screenY0;
        courseF4->y1 = screenY1;
        courseF4->y2 = screenY2;
        courseF4->y3 = screenY3;
        courseF4->r0 = (u8) g_EnvironmentColors.fields.slots[ENV_SKY_BOTTOM].cur.bytes.r;
        courseF4->g0 = (u8) g_EnvironmentColors.fields.slots[ENV_SKY_BOTTOM].cur.bytes.g;
        courseF4->b0 =
            g_EnvironmentColors.fields.slots[ENV_SKY_BOTTOM].cur.bytes.b;
        AddPrim(&work->orderingTable[SKY_OT_NEAR], courseF4);
        packetAddress.polyF4 = courseF4 + 1;
        packetCursor = packetAddress.bytes;
      }
  return packetCursor;
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
static void MeasureSkyBand(SkyRenderWork *work,
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

static void InitializeSkyRenderWork(SkyRenderWork *work) {
    work->packetCursor = RENDER_PRIM_CURSOR_AS(u8);
    work->orderingTable = RENDER_OT_BASE_AS(OT_TYPE);
    work->cameraX = g_RenderState.viewX;
    work->cameraY = g_RenderState.viewY;
    work->cameraZ = g_RenderState.viewZ;
    work->pitch = g_RenderState.viewAngleX;
    work->yaw = g_RenderState.viewAngleY;
    work->roll = g_RenderState.viewAngleZ;
    work->mirrorFlag = g_RenderState.orderingFlag;
}

static u8 *DrawTexturedSkyGrid(SkyRenderWork *work,
                               const SkyBandSetup *band,
                               s32 screenX[4],
                               u8 *packetCursor) {
    s32 rowShearX = 0;
    s32 rowShearY = 0;

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

static u8 *DrawHorizonTileStrip(SkyRenderWork *work,
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

static u8 *DrawSkyGradientQuad(SkyRenderWork *work,
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

static u8 *DrawSkyGradientBands(SkyRenderWork *work,
                                const SkyBandSetup *band,
                                s32 finalScreenX[4],
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

    for (s32 corner = 0; corner < 4; corner++) {
        finalScreenX[corner] = screenX[corner];
    }
    return packetCursor;
}

void DrawSkyBackground(void)
{
  SkyRenderWork skyWork;
  SkyRenderWork *work = &skyWork;
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
  s32 screenX0;
  s32 screenX1;
  s32 screenX2;
  s32 screenX3;
  InitializeSkyRenderWork(work);
  u8 *packetCursor = work->packetCursor;
  s32 lowerPanelXFixed;
  s32 coordinateAccumulator;
  {
    SkyBandSetup band;

    MeasureSkyBand(work, &band);
    panelXFixed = band.panelXFixed;
    panelYFixed = band.panelYFixed;
    columnStepX = band.columnStepX;
    columnStepY = band.columnStepY;
    rowStepX = band.rowStepX;
    rowStepY = band.rowStepY;
    savedSinRoll = band.savedSinRoll;
    savedCosRoll = band.savedCosRoll;
    textureColumn = band.textureColumn;
    bandOriginXFixed = band.bandOriginXFixed;
    bandOriginYFixed = band.bandOriginYFixed;
    lowerPanelXFixed = band.lowerPanelXFixed;
    coordinateAccumulator = band.coordinateAccumulator;
  }
    if (g_SkyRowBase != 0)
    {
      SkyBandSetup band;
      s32 gridScreenX[4];

      band.panelXFixed = panelXFixed;
      band.panelYFixed = panelYFixed;
      band.columnStepX = columnStepX;
      band.columnStepY = columnStepY;
      band.rowStepX = rowStepX;
      band.rowStepY = rowStepY;
      band.textureColumn = textureColumn;
      packetCursor = DrawTexturedSkyGrid(work, &band, gridScreenX, packetCursor);
      screenX0 = gridScreenX[0];
      screenX1 = gridScreenX[1];
      screenX2 = gridScreenX[2];
      screenX3 = gridScreenX[3];
      columnStepX *= 8;
      columnStepY *= 8;
    }
    else
    {
      SkyBandSetup band;
      s32 gradientScreenX[4];

      band.lowerPanelXFixed = lowerPanelXFixed;
      band.coordinateAccumulator = coordinateAccumulator;
      band.bandOriginXFixed = bandOriginXFixed;
      band.bandOriginYFixed = bandOriginYFixed;
      band.columnStepX = columnStepX;
      band.columnStepY = columnStepY;
      band.rowStepX = rowStepX;
      band.rowStepY = rowStepY;
      band.textureColumn = textureColumn;
      packetCursor = DrawHorizonTileStrip(work, &band, packetCursor);
      packetCursor = DrawSkyGradientBands(
          work, &band, gradientScreenX, packetCursor);
      panelXFixed = bandOriginXFixed;
      panelYFixed = bandOriginYFixed;
      columnStepX *= 8;
      columnStepY *= 8;
      screenX0 = gradientScreenX[0];
      screenX1 = gradientScreenX[1];
      screenX2 = gradientScreenX[2];
      screenX3 = gradientScreenX[3];
    }
  {
    SkyBandGeometry band;

    band.panelX = panelXFixed;
    band.panelY = panelYFixed;
    band.columnStepX = columnStepX;
    band.columnStepY = columnStepY;
    band.rowStepX = rowStepX;
    band.rowStepY = rowStepY;
    band.sinRoll = savedSinRoll;
    band.cosRoll = savedCosRoll;
    band.textureColumn = textureColumn;
    band.screenX0 = screenX0;
    band.screenX1 = screenX1;
    band.screenX2 = screenX2;
    band.screenX3 = screenX3;

    packetCursor = DrawCourseSkirt(work, &band, packetCursor);
    RENDER_PRIM_CURSOR_AS(u8) = packetCursor;
  }
}
