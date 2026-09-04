#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track_internal.h"
#include "game/terrain_internal.h"
#include "rage/render_world_game.h"

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

enum {
    SKY_TEXTURE_PAGE = 0x18,
    SKY_TEXTURE_CLUT = 0x798E,
    SKY_TEXTURE_NEUTRAL_COLOR = 0x80,
    /* Keep enough correctly indexed source geometry for ultrawide native
     * viewports. The PS1 draw area clips the surplus in the classic path. */
    SKY_GRID_COLUMNS = 24,
    SKY_GRID_LEFT_COLUMNS = 8,
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
    u8 *packet;
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
static u8 *DrawFarGroundGradient(
    SkyFrame *frame, const SkyBandGeometry *band, SkySkirtEdge *bottomEdge,
    u8 *packet) {
    POLY_G4 *quad = (POLY_G4 *)packet;
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
    return (u8 *)(quad + 1);
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

static u8 *DrawNearGroundGradient(
    SkyFrame *frame, const SkyBandGeometry *band,
    const SkySkirtEdge *bottomEdge, u8 *packet) {
    POLY_G4 *quad = (POLY_G4 *)packet;

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
    return (u8 *)(quad + 1);
}

static u8 *DrawFlatCourseSkirt(
    SkyFrame *frame, const SkySkirtEdge *topEdge,
    const SkySkirtEdge *bottomEdge, u8 *packet) {
    POLY_F4 *quad = (POLY_F4 *)packet;
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
    return (u8 *)(quad + 1);
}

static u8 *DrawCourseSkirt(SkyFrame *frame,
                           const SkyBandGeometry *band, u8 *packet) {
    SkySkirtEdge bottomEdge = MeasureLowerSkirtEdge(band);

    if (g_CourseIndex == 2) {
        return DrawNearGroundGradient(frame, band, &bottomEdge, packet);
    }

    SkySkirtEdge topEdge;
    packet = DrawFarGroundGradient(frame, band, &topEdge, packet);
    return DrawFlatCourseSkirt(frame, &topEdge, &bottomEdge, packet);
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

void MeasureSkyGridLayout(s32 cameraY, s32 sourcePitch, s32 sourceYaw,
                          s32 sourceRoll, s32 orderingFlag, s32 mirrorMode,
                          GameSkyGridLayout *band) {
    s32 pitch = orderingFlag != mirrorMode ? -sourcePitch : sourcePitch;
    s32 cameraPitch = SignedAngle12(pitch) + 2 +
                      DivideBy32TowardZero(cameraY - 6000);
    s32 yaw = orderingFlag != 0 ? -sourceYaw : sourceYaw;
    s32 yawAngle = (yaw + 0x200) & 0xFFF;
    s32 nearVerticalFixed = (-0x80 - cameraPitch / 2) * 256;
    s32 farVerticalFixed = (-0x80 - (cameraPitch / 2 + 0x50)) * 256;
    s32 horizontalFixed = (-0x100 - ((yawAngle >> 1) & 0x3F)) * 256;
    s32 rollAngle = mirrorMode == 0 ? -sourceRoll : sourceRoll;
    s32 sinRoll = rsin(rollAngle);
    s32 cosRoll = rcos(rollAngle);
    s32 rotatedHorizontalY = -sinRoll * horizontalFixed;
    s32 nearX = cosRoll * horizontalFixed + sinRoll * nearVerticalFixed;
    s32 nearY = rotatedHorizontalY + cosRoll * nearVerticalFixed;
    s32 farX = cosRoll * horizontalFixed + sinRoll * farVerticalFixed;
    s32 farY = rotatedHorizontalY + cosRoll * farVerticalFixed;
    s32 verticalOrigin = mirrorMode != orderingFlag ? 0x2400 : 0x7800;

    band->panelXFixed = nearX / 4096 + 0xA000;
    band->panelYFixed = nearY / 4096 + verticalOrigin;
    band->lowerPanelXFixed = farX / 4096 + 0xA000;
    band->lowerPanelYFixed = farY / 4096 + verticalOrigin;
    band->columnStepX = cosRoll * 4;
    band->columnStepY = -sinRoll * 4;
    band->rowStepX = sinRoll * 8;
    band->rowStepY = cosRoll * 8;
    band->textureColumn = yawAngle >> 7;
}

static void MeasureSkyBand(const SkyFrame *frame, SkyBandSetup *band) {
    GameSkyGridLayout layout;
    MeasureSkyGridLayout(frame->cameraY, frame->pitch, frame->yaw, frame->roll,
                         frame->mirrorFlag, g_MirrorMode, &layout);
    band->panelXFixed = layout.panelXFixed;
    band->panelYFixed = layout.panelYFixed;
    band->lowerPanelXFixed = layout.lowerPanelXFixed;
    band->lowerPanelYFixed = layout.lowerPanelYFixed;
    band->columnStepX = layout.columnStepX;
    band->columnStepY = layout.columnStepY;
    band->rowStepX = layout.rowStepX;
    band->rowStepY = layout.rowStepY;
    band->textureColumn = layout.textureColumn;
    band->sinRoll = layout.rowStepX / 8;
    band->cosRoll = layout.rowStepY / 8;
}

static void InitializeSkyFrame(SkyFrame *work) {
    work->packet = RENDER_PRIM_CURSOR_AS(u8);
    work->orderingTable = RENDER_OT_BASE;
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

static void InitializeTexturedSkyQuad(POLY_FT4 *quad,
                                      const SkyTileUV *tile) {
    SetPolyFT4(quad);
    SetShadeTex(quad, 0);
    quad->tpage = SKY_TEXTURE_PAGE;
    quad->clut = SKY_TEXTURE_CLUT;
    quad->r0 = SKY_TEXTURE_NEUTRAL_COLOR;
    quad->g0 = SKY_TEXTURE_NEUTRAL_COLOR;
    quad->b0 = SKY_TEXTURE_NEUTRAL_COLOR;
    SetSkyQuadUV(quad, tile);
}

static const SkyTileUV *SkyTileAt(s32 row, s32 column) {
    s32 tileIndex;

    if ((u32)row >= SKY_TILE_MAP_ROWS) {
        return &g_SkyTileUV[0];
    }
    tileIndex = g_SkyTileMap[row][column & (SKY_TILE_MAP_COLUMNS - 1)];
    if ((u32)tileIndex >= SKY_TILE_COUNT) {
        tileIndex = 0;
    }
    return &g_SkyTileUV[tileIndex];
}

static u8 *DrawTexturedSkyGrid(SkyFrame *work, const SkyBandSetup *band,
                               u8 *packet) {
    s32 rowShearX = 0;
    s32 rowShearY = 0;
    s32 screenX[4];

    for (s32 row = 0; row < 4; row++) {
        s32 cellX = band->panelXFixed -
                    band->columnStepX * SKY_GRID_LEFT_COLUMNS;
        s32 cellY = band->panelYFixed -
                    band->columnStepY * SKY_GRID_LEFT_COLUMNS;

        for (s32 column = 0; column < SKY_GRID_COLUMNS; column++) {
            POLY_FT4 *quad = (POLY_FT4 *)packet;
            const SkyTileUV *tileUv = SkyTileAt(
                WrapSigned32((int64_t)(row & 1) + g_SkyRowBase),
                band->textureColumn + column - SKY_GRID_LEFT_COLUMNS);
            s32 nextCellX = cellX + band->columnStepX;
            s32 nextCellY = cellY + band->columnStepY;
            s32 leftX = cellX - rowShearX;
            s32 rightX = nextCellX - rowShearX;
            s32 topY = cellY - rowShearY;
            s32 bottomY = nextCellY - rowShearY;
            InitializeTexturedSkyQuad(quad, tileUv);
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
            AddPrim(&work->orderingTable[SKY_OT_NEAR], quad);

            packet = (u8 *)(quad + 1);
            cellX = nextCellX;
            cellY = nextCellY;
        }

        rowShearX += band->rowStepX;
        rowShearY += band->rowStepY;
    }

    return packet;
}

static u8 *DrawHorizonTileStrip(SkyFrame *work, const SkyBandSetup *band,
                                u8 *packet) {
    s32 panelX = band->lowerPanelXFixed -
                 band->columnStepX * SKY_GRID_LEFT_COLUMNS;
    s32 panelY = band->lowerPanelYFixed -
                 band->columnStepY * SKY_GRID_LEFT_COLUMNS;

    for (s32 column = 0; column < SKY_GRID_COLUMNS; column++) {
        s32 nextPanelX = panelX + band->columnStepX;
        s32 nextPanelY = panelY + band->columnStepY;
        s32 screenX[4] = {
            Fixed8ToScreen(panelX),
            Fixed8ToScreen(nextPanelX),
            Fixed8ToScreen(panelX + band->rowStepX),
            Fixed8ToScreen(nextPanelX + band->rowStepX),
        };

        /* Classic clips these against its 320-pixel draw area. Modern keeps
         * the off-screen reserve for wider presentation targets. */
        {
            POLY_FT4 *quad = (POLY_FT4 *)packet;
            const SkyTileUV *tileUv =
                SkyTileAt(0, band->textureColumn + column -
                                 SKY_GRID_LEFT_COLUMNS);
            InitializeTexturedSkyQuad(quad, tileUv);
            quad->x0 = screenX[0];
            quad->x1 = screenX[1];
            quad->x2 = screenX[2];
            quad->x3 = screenX[3];
            quad->y0 = Fixed8ToScreen(panelY);
            quad->y1 = Fixed8ToScreen(nextPanelY);
            quad->y2 = Fixed8ToScreen(panelY + band->rowStepY);
            quad->y3 = Fixed8ToScreen(nextPanelY + band->rowStepY);
            AddPrim(&work->orderingTable[SKY_OT_NEAR], quad);
            packet = (u8 *)(quad + 1);
        }

        panelX = nextPanelX;
        panelY = nextPanelY;
    }

    return packet;
}

static u8 *DrawSkyGradientQuad(
    SkyFrame *work, u8 *packet, const s32 screenX[4],
    const s32 screenY[4], GameEnvColor nearColor, GameEnvColor farColor) {
    POLY_G4 *quad = (POLY_G4 *)packet;

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
    return (u8 *)(quad + 1);
}

static u8 *DrawSkyGradientBands(SkyFrame *work, const SkyBandSetup *band,
                                u8 *packet) {
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

    packet = DrawSkyGradientQuad(
        work, packet, screenX, screenY,
        g_EnvironmentColors.fields.slots[ENV_SKY_MIDDLE].cur,
        g_EnvironmentColors.fields.slots[ENV_SKY_HORIZON].cur);

    screenX[2] = Fixed8ToScreen(panelX - band->rowStepX);
    screenX[3] = Fixed8ToScreen(bandRightX - band->rowStepX);
    screenY[2] = Fixed8ToScreen(panelY - band->rowStepY);
    screenY[3] = Fixed8ToScreen(bandBottomY - band->rowStepY);
    packet = DrawSkyGradientQuad(
        work, packet, screenX, screenY,
        g_EnvironmentColors.fields.slots[ENV_SKY_MIDDLE].cur,
        g_EnvironmentColors.fields.slots[ENV_SKY_TOP].cur);

    screenX[1] = screenX[3];
    screenX[2] = Fixed8ToScreen(panelX - band->rowStepX * 3);
    screenX[3] = Fixed8ToScreen(bandRightX - band->rowStepX * 3);
    screenY[0] = screenY[2];
    screenY[1] = screenY[3];
    screenY[2] = Fixed8ToScreen(panelY - band->rowStepY * 3);
    screenY[3] = Fixed8ToScreen(bandBottomY - band->rowStepY * 3);
    packet = DrawSkyGradientQuad(
        work, packet, screenX, screenY,
        g_EnvironmentColors.fields.slots[ENV_SKY_TOP].cur, darkSky);

    return packet;
}

void DrawSkyBackground(void) {
    SkyFrame work;
    SkyBandSetup setup;
    SkyBandGeometry geometry;
    u8 *packet;

    InitializeSkyFrame(&work);
    MeasureSkyBand(&work, &setup);
    {
        GameSkyGridLayout exact = {
            setup.panelXFixed, setup.panelYFixed,
            setup.lowerPanelXFixed, setup.lowerPanelYFixed,
            setup.columnStepX, setup.columnStepY,
            setup.rowStepX, setup.rowStepY, setup.textureColumn,
        };
        GameRenderWorldSetSkyGrid(&exact,
                                  work.mirrorFlag != g_MirrorMode);
    }
    packet = work.packet;
    GameRenderWorldBeginSkyPackets();

    geometry.panelX = setup.panelXFixed;
    geometry.panelY = setup.panelYFixed;
    geometry.columnStepX = setup.columnStepX * 8;
    geometry.columnStepY = setup.columnStepY * 8;
    geometry.rowStepX = setup.rowStepX;
    geometry.rowStepY = setup.rowStepY;
    geometry.sinRoll = setup.sinRoll;
    geometry.cosRoll = setup.cosRoll;

    if (g_SkyRowBase != 0) {
        packet = DrawTexturedSkyGrid(&work, &setup, packet);
    } else {
        packet = DrawHorizonTileStrip(&work, &setup, packet);
        packet = DrawSkyGradientBands(&work, &setup, packet);
    }

    packet = DrawCourseSkirt(&work, &geometry, packet);
    g_RenderState.packetCursor = packet;
    GameRenderWorldEndSkyPackets();
}
