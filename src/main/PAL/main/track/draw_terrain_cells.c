#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track_internal.h"
#include "game/terrain_internal.h"


void DrawTerrainCells(void) {
    BuildVisibleCells(-12288, 0x14000);
    SetRotMatrix(SCRATCH_VIEW_MATRIX_GTE);
    SubmitTerrainCells(SCRATCHPAD, g_VisibleCellList, 0x40);
}

void DrawTerrainCellsWide(void) {
    BuildVisibleCells(0xFFFF6000, 0x14000);
    SetRotMatrix(SCRATCH_VIEW_MATRIX_GTE);
    SubmitTerrainCells(SCRATCHPAD, g_VisibleCellList, 0x40);
}

static s32 DivideSigned32(s32 value)
{
  s32 adjustedValue = value;
  s32 divisionInput = value;
  if (divisionInput < 0)
    adjustedValue += 31;
  return adjustedValue >> 5;
}

static s32 GameRoundTerrainCoordinate(s32 value)
{
  return value / 256;
}

static s32 GameRoundTerrainCoordinate11(s32 value)
{
  return value / 2048;
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
    s32 courseX0;
    s32 courseX1;
    s32 courseY1;
    s32 xWorkLate;
} SkyBandGeometry;

/*
 * The skirt under the sky: the band of quads that closes the gap between the
 * horizon and the terrain. Two courses want it drawn differently, which is the
 * only thing the course index decides here. Returns where it left the packet
 * cursor.
 */
static u8 *DrawCourseSkirt(SkyRenderScratchpad *scratch, SkyBandGeometry *band,
                           u8 *packetCursor) {
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
        u8 color;
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
        band->courseX0 = band->screenX2;
        band->screenX3 = GameRoundTerrainCoordinate11(rightXWorkFixed + band->sinRoll);
        courseTopY = (band->panelY + band->rowStepY) * 8;
        band->courseX1 = band->screenX3;
        screenY0 = GameRoundTerrainCoordinate11(courseTopY - band->cosRoll);
        courseBottomY = ((band->panelY + band->columnStepY) + band->rowStepY) * 8;
        screenY1 = GameRoundTerrainCoordinate11(courseBottomY - band->cosRoll);
        screenY2 = GameRoundTerrainCoordinate11(courseTopY + band->cosRoll);
        band->xWorkLate = screenY2;
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
        color = g_EnvironmentColors.fields.slots[7].cur.bytes.r;
        courseG4->r1 = color;
        courseG4->r0 = color;
        color = g_EnvironmentColors.fields.slots[8].cur.bytes.r;
        courseG4->r3 = color;
        courseG4->r2 = color;
        color = g_EnvironmentColors.fields.slots[7].cur.bytes.g;
        courseG4->g1 = color;
        courseG4->g0 = color;
        band->courseY1 = screenY3;
        color = g_EnvironmentColors.fields.slots[8].cur.bytes.g;
        courseG4->g3 = color;
        courseG4->g2 = color;
        color = g_EnvironmentColors.fields.slots[7].cur.bytes.b;
        courseG4->b1 = color;
        courseG4->b0 = color;
        cursor.polyG4 = courseG4 + 1;
        nextPacket = cursor.bytes;
        color = g_EnvironmentColors.fields.slots[8].cur.bytes.b;
        courseG4->b3 = color;
        courseG4->b2 = color;
        AddPrim(&scratch->orderingTable[SKY_OT_FAR], courseG4);
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
        u8 color;
        POLY_G4 *courseG4;
        RenderBufferAddress cursor;
        cursor.bytes = packetCursor;
        courseG4 = cursor.polyG4;
        rightXWorkFixed = band->panelX;
        band->screenX0 = GameRoundTerrainCoordinate(rightXWorkFixed + band->rowStepX);
        band->screenX1 = GameRoundTerrainCoordinate(skirtRightX + (band->rowStepX + (band->rowStepX - band->rowStepX)));
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
        color = g_EnvironmentColors.fields.slots[5].cur.bytes.r;
        courseG4->r1 = color;
        courseG4->r0 = color;
        color = g_EnvironmentColors.fields.slots[6].cur.bytes.r;
        courseG4->r3 = color;
        courseG4->r2 = color;
        color = g_EnvironmentColors.fields.slots[5].cur.bytes.g;
        courseG4->g1 = color;
        courseG4->g0 = color;
        color = g_EnvironmentColors.fields.slots[6].cur.bytes.g;
        courseG4->g3 = color;
        courseG4->g2 = color;
        color = g_EnvironmentColors.fields.slots[5].cur.bytes.b;
        courseG4->b1 = color;
        courseG4->b0 = color;
        color = g_EnvironmentColors.fields.slots[6].cur.bytes.b;
        courseG4->b3 = color;
        courseG4->b2 = color;
        AddPrim(&scratch->orderingTable[SKY_OT_NEAR], courseG4);
        packetCursor = nextPacket;
      }
      else
      {
        POLY_F4 *courseF4;
        packetAddress.bytes = packetCursor;
        courseF4 = packetAddress.polyF4;
        band->screenX0 = band->courseX0;
        band->screenX1 = band->courseX1;
        screenY0 = band->xWorkLate;
        screenY1 = band->courseY1;
        SetPolyF4(courseF4);
        courseF4->x0 = band->screenX0;
        courseF4->x1 = band->screenX1;
        courseF4->x2 = band->screenX2;
        courseF4->x3 = band->screenX3;
        courseF4->y0 = screenY0;
        courseF4->y1 = screenY1;
        courseF4->y2 = screenY2;
        courseF4->y3 = screenY3;
        courseF4->r0 = (u8) g_EnvironmentColors.fields.slots[4].cur.bytes.r;
        courseF4->g0 = (u8) g_EnvironmentColors.fields.slots[4].cur.bytes.g;
        {
          u8 *nextPacket;
          RenderBufferAddress cursor;
          cursor.polyF4 = courseF4 + 1;
          nextPacket = cursor.bytes;
          courseF4->b0 = (u8) g_EnvironmentColors.fields.slots[4].cur.bytes.b;
          AddPrim(&scratch->orderingTable[SKY_OT_NEAR], courseF4);
          packetCursor = nextPacket;
        }
      }
  return packetCursor;
}

void DrawSkyBackground(void)
{
  SkyRenderScratchpad nativeScratch;
  SkyRenderScratchpad *scratch = &nativeScratch;
  s32 panelXFixed;
  s32 panelYFixed;
  s32 columnStepX;
  s32 columnStepY;
  s32 rowStepX;
  s32 rowStepY;
  s32 heldScreenY;
  s32 savedSinRoll;
  s32 savedCosRoll;
  s32 textureColumn;
  s32 bandOriginXFixed;
  s32 bandOriginYFixed;
  s32 horizonTopY;
  s32 screenX0;
  s32 screenX1;
  s32 screenX2;
  s32 screenX3;
  s32 savedCourseX0;
  scratch->packetCursor = SCRATCH_PRIM_CURSOR_AS(u8);
  scratch->orderingTable = SCRATCH_OT_BASE_AS(OT_TYPE);
  scratch->cameraX = SCRATCH_VIEW_X;
  scratch->cameraY = SCRATCH_VIEW_Y;
  scratch->cameraZ = SCRATCH_VIEW_Z;
  scratch->pitch = SCRATCH_VIEW_ANGLE_X;
  scratch->yaw = SCRATCH_VIEW_ANGLE_Y;
  scratch->roll = SCRATCH_VIEW_ANGLE_Z;
  scratch->mirrorFlag = SCRATCH_MIRROR;
  u8 *packetCursor = scratch->packetCursor;
  s32 savedCourseX1;
  s32 heldBandY;
  s32 xWork_late;
  s32 adjW;
  s32 courseSaveY1;
  s32 doubleRowStepY;
  s32 nextCellXFixed;
  s32 rowOffsetYFixed;
  s32 xWork;
  s32 upperBandYFixed;
  s32 savedCourseY1;
  s32 lowerPanelXFixed;
  s32 cellXFixed;
  s32 coordinateAccumulator;
  {
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
    if (scratch->mirrorFlag != g_MirrorMode)
    {
      pitchAngle = -scratch->pitch;
    }
    else
    {
      pitchAngle = scratch->pitch;
    }
    coordinateAccumulator = pitchAngle & 0xFFF;
    leftViewAngle = coordinateAccumulator;
    if (leftViewAngle >= 0x800)
    {
      leftViewAngle -= 0x1000;
    }
    if (coordinateAccumulator >= 0x800)
    {
      coordinateAccumulator -= 0x1000;
    }
    cameraY = scratch->cameraY;
    {
      s32 leftPlusTwo;
      s32 rightPlusTwo;
      angleWork = cameraY - 6000;
      leftPlusTwo = leftViewAngle + 2;
      cellXFixed = DivideSigned32(angleWork);
      leftViewAngle = leftPlusTwo + cellXFixed;
      rightPlusTwo = coordinateAccumulator + 2;
      coordinateAccumulator = rightPlusTwo + cellXFixed;
      yawAngle = scratch->mirrorFlag;
    }
    angleWork = scratch->yaw;
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
    coordinateAccumulator = (coordinateAccumulator / 2) + 0x50;
    textureColumn = angleWork >> 7;
    horizontalFixed = ((u32)((-0x100) - ((angleWork >> 1) & 0x3F))) << 8;
    nearVerticalFixed = ((u32)((-0x80) - leftViewAngle)) << 8;
    rollAngle = scratch->roll;
    farVerticalFixed = ((u32)((-0x80) - coordinateAccumulator)) << 8;
    if (g_MirrorMode == 0)
    {
      rollAngle = -rollAngle;
    }
    sinRoll = rsin(rollAngle);
    cosRoll = rcos(rollAngle);
    coordinateAccumulator = cosRoll * horizontalFixed;
    rotatedX = coordinateAccumulator + (sinRoll * nearVerticalFixed);
    unroundedX = rotatedX;
    if (unroundedX < 0)
    {
      rotatedX += 0xFFF;
    }
    negativeSinRoll = -sinRoll;
    rotatedBandY = negativeSinRoll * horizontalFixed;
    rotatedX >>= 0xC;
    leftViewAngle = 0xA000;
    panelXFixed = rotatedX + leftViewAngle;
    rotatedY = rotatedBandY + (cosRoll * nearVerticalFixed);
    unroundedY = rotatedY;
    if (unroundedY < 0)
    {
      rotatedY += 0xFFF;
    }
    bandRowY = rotatedY >> 0xC;
    coordinateAccumulator += sinRoll * farVerticalFixed;
    panelYFixed = bandRowY + 0x7800;
    if (coordinateAccumulator < 0)
    {
      coordinateAccumulator += 0xFFF;
    }
    rotatedY = cosRoll * farVerticalFixed;
    rotatedBandY += rotatedY;
    rotatedY = coordinateAccumulator >> 0xC;
    lowerPanelXFixed = rotatedY + 0xA000;
    if (rotatedBandY < 0)
    {
      rotatedBandY += 0xFFF;
    }
    leftViewAngle = rotatedBandY >> 0xC;
    coordinateAccumulator = leftViewAngle + 0x7800;
    if (g_MirrorMode != SCRATCH_MIRROR)
    {
      panelYFixed = 0x2400;
      panelYFixed = bandRowY + panelYFixed;
      coordinateAccumulator = leftViewAngle + 0x2400;
    }
    columnStepX = cosRoll * 4;
    columnStepY = negativeSinRoll * 4;
    savedSinRoll = sinRoll;
    savedCosRoll = cosRoll;
    bandOriginXFixed = panelXFixed;
    bandOriginYFixed = panelYFixed;
    rowStepX = sinRoll * 8;
    rowStepY = cosRoll * 8;
  }
  {
    s32 screenY3;
    s32 screenY2;
    s32 screenY1;
    s32 screenY0;
    s32 gridRow;
    s32 leftXWorkFixed;
    s32 rightXWorkFixed;
  SkyTileUV *tileUv;
  u8 *nextPacket;
  RenderBufferAddress packetAddress;
    if (g_SkyRowBase != 0)
    {
      {
        s32 nextTileY;
        s32 bandRowY;
        POLY_FT4 *quad;
        GpuUvAddress uvAddress;
        POLY_FT4 *quadRow;
        s16 tileIndex;
        s32 tileBottomY;
        s32 tileRightX;
        s32 tileLeftX;
        s32 tileTopY;
        s32 column;
        s32 rowShearY = 0;
        s32 rowShearX = 0;
        gridRow = 0;
        do
        {
          column = 0;
          packetAddress.bytes = packetCursor;
          quadRow = packetAddress.polyFT4;
          doubleRowStepY = rowShearX;
          rowOffsetYFixed = rowShearY;
          bandRowY = panelYFixed;
          cellXFixed = panelXFixed;
          do
          {
            quad = quadRow + column;
            tileIndex = g_SkyTileMap[(gridRow % 2) + g_SkyRowBase][(textureColumn + column) & 0xF];
            tileUv = &g_SkyTileUV[tileIndex];
            tileLeftX = cellXFixed - doubleRowStepY;
            screenX0 = GameRoundTerrainCoordinate(tileLeftX);
            nextCellXFixed = cellXFixed + columnStepX;
            tileRightX = nextCellXFixed - doubleRowStepY;
            screenX1 = GameRoundTerrainCoordinate(tileRightX);
            screenX2 = GameRoundTerrainCoordinate(tileLeftX + rowStepX);
            screenX3 = GameRoundTerrainCoordinate(tileRightX + rowStepX);
            tileTopY = bandRowY - rowOffsetYFixed;
            screenY0 = GameRoundTerrainCoordinate(tileTopY);
            nextTileY = bandRowY + columnStepY;
            tileBottomY = nextTileY - rowOffsetYFixed;
            screenY1 = GameRoundTerrainCoordinate(tileBottomY);
            screenY2 = GameRoundTerrainCoordinate(tileTopY + rowStepY);
            screenY3 = GameRoundTerrainCoordinate(tileBottomY + rowStepY);
            SetPolyFT4(packetCursor);
            SetShadeTex(packetCursor, 0);
            quad->tpage = 0x18;
            uvAddress.bytes = &quad->u0;
            *uvAddress.packed = tileUv->corner[0].packed;
            packetCursor += sizeof(POLY_FT4);
            uvAddress.bytes = &quad->u1;
            *uvAddress.packed = tileUv->corner[1].packed;
            uvAddress.bytes = &quad->u2;
            *uvAddress.packed = tileUv->corner[2].packed;
            uvAddress.bytes = &quad->u3;
            *uvAddress.packed = tileUv->corner[3].packed;
            quad->x0 = screenX0;
            quad->x1 = screenX1;
            quad->x2 = screenX2;
            quad->x3 = screenX3;
            quad->r0 = 0x80;
            quad->g0 = 0x80;
            heldScreenY = screenY0;
            quad->b0 = 0x80;
            quad->y0 = heldScreenY;
            quad->y1 = screenY1;
            quad->y2 = screenY2;
            quad->y3 = screenY3;
            quad->clut = 0x798E;
            AddPrim(&scratch->orderingTable[SKY_OT_NEAR], quad);
            column += 1;
            cellXFixed = nextCellXFixed;
            bandRowY = nextTileY;
          }
          while (column < 8);
          gridRow += 1;
          rowShearY += rowStepY;
          rowShearX += rowStepX;
        }
        while (gridRow < 4);
      }
      columnStepX *= 8;
      columnStepY *= 8;
    }
    else
    {
      SkyClipBounds clip;
      s32 rotatedBandY;
      POLY_FT4 *quad;
      GpuUvAddress uvAddress;
      s32 bandRightX;
      s32 stripFarX;
      s32 stripRightX;
      s32 stripLowerY;
      s32 horizonBottomY;
      s32 lowestY;
      s32 upperBandXFixed;
      s32 bandNextY;
      panelYFixed = coordinateAccumulator;
      {
        panelXFixed = lowerPanelXFixed;
        horizonTopY = GameRoundTerrainCoordinate(panelYFixed);
        horizonBottomY = GameRoundTerrainCoordinate(panelYFixed + rowStepY);
        lowestY = 0xF0;
        clip.xMinTop = (clip.xMinBottom = 0x140);
        clip.xMaxBottom = 0;
        clip.xMaxTop = 0;
        clip.yEdge0 = (columnStepY > 0) ? (0xF0) : (-0xF0);
        clip.yEdge1 = (columnStepY > 0) ? (-0xF0) : (0xF0);
        clip.yEdge2 = (columnStepY > 0) ? (0xF0) : (-0xF0);
        clip.yEdge3 = (columnStepY > 0) ? (-0xF0) : (0xF0);
        gridRow = 0;
        packetAddress.bytes = packetCursor;
        quad = packetAddress.polyFT4;
        do
        {
          screenX0 = GameRoundTerrainCoordinate(panelXFixed);
          stripRightX = panelXFixed + columnStepX;
          screenX1 = GameRoundTerrainCoordinate(stripRightX);
          screenX2 = GameRoundTerrainCoordinate(panelXFixed + rowStepX);
          stripFarX = GameRoundTerrainCoordinate(stripRightX + rowStepX);
          screenX3 = stripFarX;
          if (((((screenX0 >= 0) || (screenX1 >= 0)) || (screenX2 >= 0)) || (stripFarX >= 0)) && ((((screenX0 < 0x140) || (screenX1 < 0x140)) || (screenX2 < 0x140)) || (screenX3 < 0x140)))
          {
            screenY0 = GameRoundTerrainCoordinate(panelYFixed);
            stripLowerY = panelYFixed + columnStepY;
            screenY1 = GameRoundTerrainCoordinate(stripLowerY);
            screenY2 = GameRoundTerrainCoordinate(panelYFixed + rowStepY);
            screenY3 = GameRoundTerrainCoordinate(stripLowerY + rowStepY);
            if (horizonTopY < screenY0)
            {
              horizonTopY = screenY0;
            }
            if (horizonTopY < screenY1)
            {
              horizonTopY = screenY1;
            }
            if (screenY2 < horizonBottomY)
            {
              horizonBottomY = screenY2;
            }
            if (screenY3 < horizonBottomY)
            {
              horizonBottomY = screenY3;
            }
            if (screenY2 < screenY3)
            {
              if (screenY2 < lowestY)
              {
                lowestY = screenY2;
              }
            }
            else
              if (screenY3 < lowestY)
            {
              lowestY = screenY3;
            }
            if (screenX0 < clip.xMinTop)
            {
              clip.xMinTop = screenX0;
            }
            if (screenX2 < clip.xMinBottom)
            {
              clip.xMinBottom = screenX2;
            }
            if (clip.xMaxTop < screenX1)
            {
              clip.xMaxTop = screenX1;
            }
            if (clip.xMaxBottom < screenX3)
            {
              clip.xMaxBottom = screenX3;
            }
            if (0 < columnStepY)
            {
              if (screenY0 < clip.yEdge0)
              {
                clip.yEdge0 = screenY0;
              }
              if (clip.yEdge1 < screenY1)
              {
                clip.yEdge1 = screenY1;
              }
              if (screenY2 < clip.yEdge2)
              {
                clip.yEdge2 = screenY2;
              }
              if (clip.yEdge3 < screenY3)
              {
                clip.yEdge3 = screenY3;
              }
            }
            else
            {
              if (clip.yEdge0 < screenY0)
              {
                clip.yEdge0 = screenY0;
              }
              if (screenY1 < clip.yEdge1)
              {
                clip.yEdge1 = screenY1;
              }
              if (clip.yEdge2 < screenY2)
              {
                clip.yEdge2 = screenY2;
              }
              if (screenY3 < clip.yEdge3)
              {
                clip.yEdge3 = screenY3;
              }
            }
            {
              s32 lateTileIndex = g_SkyTileMap[0][(textureColumn + gridRow) & 0xF];
              tileUv = &g_SkyTileUV[lateTileIndex];
            }
            SetPolyFT4(packetCursor);
            SetShadeTex(packetCursor, 0);
            quad->tpage = 0x18;
            uvAddress.bytes = &quad->u0;
            *uvAddress.packed = tileUv->corner[0].packed;
            packetCursor += sizeof(POLY_FT4);
            uvAddress.bytes = &quad->u1;
            *uvAddress.packed = tileUv->corner[1].packed;
            uvAddress.bytes = &quad->u2;
            *uvAddress.packed = tileUv->corner[2].packed;
            uvAddress.bytes = &quad->u3;
            *uvAddress.packed = tileUv->corner[3].packed;
            quad->x0 = screenX0;
            quad->x1 = screenX1;
            quad->x2 = screenX2;
            quad->x3 = screenX3;
            quad->r0 = 0x80;
            quad->g0 = 0x80;
            quad->b0 = 0x80;
            quad->y0 = screenY0;
            quad->y1 = screenY1;
            quad->y2 = screenY2;
            quad->y3 = screenY3;
            quad->clut = 0x798E;
            AddPrim(&scratch->orderingTable[SKY_OT_NEAR], quad++);
          }
          gridRow += 1;
          panelXFixed += columnStepX;
          panelYFixed += columnStepY;
        }
        while (gridRow < 8);
      }
      panelXFixed = bandOriginXFixed;
      panelYFixed = bandOriginYFixed;
      columnStepX *= 8;
      columnStepY *= 8;
      screenX0 = GameRoundTerrainCoordinate(panelXFixed);
      bandRightX = panelXFixed + columnStepX;
      screenX1 = GameRoundTerrainCoordinate(bandRightX);
      screenX2 = GameRoundTerrainCoordinate(panelXFixed + rowStepX);
      screenX3 = GameRoundTerrainCoordinate(bandRightX + rowStepX);
      screenY0 = GameRoundTerrainCoordinate(panelYFixed);
      lowestY = panelYFixed + columnStepY;
      screenY1 = GameRoundTerrainCoordinate(lowestY);
      {
        u8 color;
        s32 bandFarY;
        POLY_G4 *firstG4;
        screenY2 = GameRoundTerrainCoordinate(panelYFixed + rowStepY);
        bandFarY = lowestY + rowStepY;
        screenY3 = bandFarY / 256;
        nextPacket = packetCursor + sizeof(POLY_G4);
        SetPolyG4(packetCursor);
        packetAddress.bytes = packetCursor;
        firstG4 = packetAddress.polyG4;
        firstG4->x0 = screenX0;
        firstG4->x1 = screenX1;
        firstG4->x2 = screenX2;
        firstG4->x3 = screenX3;
        firstG4->y0 = screenY0;
        firstG4->y1 = screenY1;
        firstG4->y2 = screenY2;
        firstG4->y3 = screenY3;
        color = g_EnvironmentColors.fields.slots[2].cur.bytes.r;
        firstG4->r1 = color;
        firstG4->r0 = color;
        color = g_EnvironmentColors.fields.slots[3].cur.bytes.r;
        firstG4->r3 = color;
        firstG4->r2 = color;
        color = g_EnvironmentColors.fields.slots[2].cur.bytes.g;
        firstG4->g1 = color;
        firstG4->g0 = color;
        color = g_EnvironmentColors.fields.slots[3].cur.bytes.g;
        firstG4->g3 = color;
        firstG4->g2 = color;
        color = g_EnvironmentColors.fields.slots[2].cur.bytes.b;
        firstG4->b1 = color;
        firstG4->b0 = color;
        color = g_EnvironmentColors.fields.slots[3].cur.bytes.b;
        firstG4->b3 = color;
        firstG4->b2 = color;
        {
          packetCursor = nextPacket;
          
          AddPrim(&scratch->orderingTable[SKY_OT_NEAR], firstG4);
        }
      }
      {
        u8 color;
        POLY_G4 *g4Cursor;
        OT_TYPE *orderingTableBase;
        RenderBufferAddress cursor;
        cursor.bytes = packetCursor;
        g4Cursor = cursor.polyG4;
        xWork = panelXFixed - rowStepX;
        adjW = xWork;
        if (xWork < 0)
        {
          adjW = xWork + 0xFF;
        }
        screenX2 = adjW >> 8;
        upperBandXFixed = bandRightX - rowStepX;
        screenX3 = upperBandXFixed / 256;
        
        upperBandYFixed = panelYFixed - rowStepY;
        adjW = upperBandYFixed;
        if (upperBandYFixed < 0)
        {
          adjW = upperBandYFixed + 0xFF;
        }
        screenY2 = adjW >> 8;
        heldBandY = lowestY;
        bandNextY = heldBandY;
        savedCourseY1 = bandNextY - rowStepY;
        adjW = savedCourseY1;
        if (savedCourseY1 < 0)
        {
          adjW = savedCourseY1 + 0xFF;
        }
        screenY3 = adjW >> 8;
        nextPacket = packetCursor + sizeof(POLY_G4);
        SetPolyG4(g4Cursor);
        g4Cursor->x0 = screenX0;
        g4Cursor->x1 = screenX1;
        g4Cursor->x2 = screenX2;
        g4Cursor->x3 = screenX3;
        g4Cursor->y0 = screenY0;
        g4Cursor->y1 = screenY1;
        g4Cursor->y2 = screenY2;
        g4Cursor->y3 = screenY3;
        color = g_EnvironmentColors.fields.slots[2].cur.bytes.r;
        g4Cursor->r1 = color;
        g4Cursor->r0 = color;
        color = g_EnvironmentColors.fields.slots[1].cur.bytes.r;
        g4Cursor->r3 = color;
        g4Cursor->r2 = color;
        color = g_EnvironmentColors.fields.slots[2].cur.bytes.g;
        g4Cursor->g1 = color;
        g4Cursor->g0 = color;
        color = g_EnvironmentColors.fields.slots[1].cur.bytes.g;
        g4Cursor->g3 = color;
        g4Cursor->g2 = color;
        color = g_EnvironmentColors.fields.slots[2].cur.bytes.b;
        g4Cursor->b1 = color;
        g4Cursor->b0 = color;
        color = g_EnvironmentColors.fields.slots[1].cur.bytes.b;
        g4Cursor->b3 = color;
        g4Cursor->b2 = color;
        orderingTableBase = scratch->orderingTable;
        AddPrim(&orderingTableBase[SKY_OT_NEAR], g4Cursor++);
        cursor.polyG4 = g4Cursor;
        nextPacket = cursor.bytes;
        
      }
      {
        u8 packetColor;
        POLY_G4 *g4Cursor;
        u16 geomValueX2;
        s32 x3Raw;
        s32 doubleStepX;
        RenderBufferAddress cursor;
        
        
        doubleStepX = rowStepX * 2;
        screenX1 = screenX3;
        leftXWorkFixed = doubleStepX + rowStepX;
        rightXWorkFixed = leftXWorkFixed;
        screenX2 = GameRoundTerrainCoordinate(panelXFixed - rightXWorkFixed);

        packetCursor = nextPacket;
        cursor.bytes = packetCursor;
        g4Cursor = cursor.polyG4;
        x3Raw = bandRightX - leftXWorkFixed;
        if (x3Raw < 0)
        {
          x3Raw += 0xFF;
        }
        
        screenX3 = x3Raw >> 8;;
        
        screenY0 = screenY2;
        rotatedBandY = rowStepY * 3;
        screenY1 = screenY3;
        screenY2 = GameRoundTerrainCoordinate(panelYFixed - rotatedBandY);
        screenY3 = GameRoundTerrainCoordinate(heldBandY - rotatedBandY);
        cursor.polyG4 = g4Cursor + 1;
        nextPacket = cursor.bytes;
        SetPolyG4(g4Cursor);
        g4Cursor->x0 = screenX0;
        g4Cursor->x1 = screenX1;
        geomValueX2 = (u16)screenX2;
        g4Cursor->x2 = geomValueX2;
        g4Cursor->x3 = screenX3;
        g4Cursor->y0 = screenY0;
        g4Cursor->y1 = screenY1;
        g4Cursor->y2 = screenY2;
        g4Cursor->y3 = screenY3;
        g4Cursor->r0 = (g4Cursor->r1 = g_EnvironmentColors.fields.slots[1].cur.bytes.r);
        g4Cursor->r2 = (g4Cursor->r3 = 0);
        {
          packetColor = g_EnvironmentColors.fields.slots[1].cur.bytes.g;
          g4Cursor->g2 = (g4Cursor->g3 = 0);
          g4Cursor->g0 = (g4Cursor->g1 = packetColor);
          {
            u8 packetColor = g_EnvironmentColors.fields.slots[1].cur.bytes.b;
            g4Cursor->b0 = (g4Cursor->b1 = packetColor);
            g4Cursor->b2 = (g4Cursor->b3 = 16);
          }
          AddPrim(&scratch->orderingTable[SKY_OT_NEAR], g4Cursor);
        }
        packetCursor = nextPacket;
      }
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
    band.courseX0 = savedCourseX0;
    band.courseX1 = savedCourseX1;
    band.courseY1 = courseSaveY1;
    band.xWorkLate = xWork_late;

    packetCursor = DrawCourseSkirt(scratch, &band, packetCursor);
    SCRATCH_PRIM_CURSOR_AS(u8) = packetCursor;
  }
  }
}
