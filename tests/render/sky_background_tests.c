#include "game/environment.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/terrain_internal.h"
#include "game/track.h"
#include "game/track_internal.h"
#include "rage/render_world_game.h"

#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;
GameEnvironmentColors g_EnvironmentColors;
s16 g_SkyTileMap[SKY_TILE_MAP_ROWS][SKY_TILE_MAP_COLUMNS];
SkyTileUV g_SkyTileUV[SKY_TILE_COUNT];
s32 g_CourseIndex;
s32 g_MirrorMode;
s32 g_SkyRowBase;
static GameSkyGridLayout s_publishedGrid;
static int s_publishedMirror;

void GameRenderWorldSetSkyGrid(const GameSkyGridLayout *layout,
                               int mirrorPass) {
    s_publishedGrid = *layout;
    s_publishedMirror = mirrorPass;
}

typedef union PacketStorage {
    max_align_t alignment;
    u8 bytes[4096];
} PacketStorage;

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void PrepareFrame(PacketStorage *packets,
                         GameOrderingTableEntry *orderingTable) {
    memset(packets, 0, sizeof(*packets));
    memset(orderingTable, 0,
           sizeof(*orderingTable) * GAME_FRAME_OT_LENGTH);
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    memset(g_SkyTileMap, 0, sizeof(g_SkyTileMap));
    memset(g_SkyTileUV, 0, sizeof(g_SkyTileUV));
    g_RenderState.packetCursor = packets->bytes;
    g_RenderState.primData = orderingTable;
    g_SkyRowBase = 1;
    g_MirrorMode = 0;
    memset(&s_publishedGrid, 0, sizeof(s_publishedGrid));
    s_publishedMirror = -1;
}

static int TestNearGroundCourseSkirt(void) {
    PacketStorage packets;
    GameOrderingTableEntry orderingTable[GAME_FRAME_OT_LENGTH];
    const size_t gridSize = 32 * sizeof(POLY_FT4);
    POLY_G4 *skirt;

    PrepareFrame(&packets, orderingTable);
    g_CourseIndex = 2;
    g_EnvironmentColors.fields.slots[ENV_GROUND_NEAR_TOP].cur.bytes.r = 11;
    g_EnvironmentColors.fields.slots[ENV_GROUND_NEAR_BOTTOM].cur.bytes.r = 22;

    DrawSkyBackground();

    CHECK(g_RenderState.packetCursor ==
          packets.bytes + gridSize + sizeof(POLY_G4));
    skirt = (POLY_G4 *)(void *)(packets.bytes + gridSize);
    CHECK(skirt->r0 == 11 && skirt->r1 == 11);
    CHECK(skirt->r2 == 22 && skirt->r3 == 22);
    return 0;
}

static int TestFarGroundCourseSkirt(void) {
    PacketStorage packets;
    GameOrderingTableEntry orderingTable[GAME_FRAME_OT_LENGTH];
    const size_t gridSize = 32 * sizeof(POLY_FT4);
    POLY_G4 *gradient;
    POLY_F4 *fill;

    PrepareFrame(&packets, orderingTable);
    g_CourseIndex = 0;
    g_EnvironmentColors.fields.slots[ENV_GROUND_FAR_TOP].cur.bytes.g = 33;
    g_EnvironmentColors.fields.slots[ENV_GROUND_FAR_BOTTOM].cur.bytes.g = 44;
    g_EnvironmentColors.fields.slots[ENV_SKY_BOTTOM].cur.bytes.b = 55;

    DrawSkyBackground();

    CHECK(g_RenderState.packetCursor ==
          packets.bytes + gridSize + sizeof(POLY_G4) + sizeof(POLY_F4));
    gradient = (POLY_G4 *)(void *)(packets.bytes + gridSize);
    fill = (POLY_F4 *)(void *)(gradient + 1);
    CHECK(gradient->g0 == 33 && gradient->g1 == 33);
    CHECK(gradient->g2 == 44 && gradient->g3 == 44);
    CHECK(fill->b0 == 55);
    return 0;
}

static int TestSkyGradientPaletteSlots(void) {
    PacketStorage packets;
    GameOrderingTableEntry orderingTable[GAME_FRAME_OT_LENGTH];
    POLY_G4 *gradient;
    POLY_FT4 *horizonTile;
    u8 *packetEnd;

    PrepareFrame(&packets, orderingTable);
    g_CourseIndex = 2;
    g_SkyRowBase = 0;
    /* Column zero is fully left of the viewport; column one is the first
     * emitted tile and samples map column five at yaw zero. */
    g_SkyTileMap[0][5] = 3;
    g_SkyTileUV[3].corner[0].bytes.u = 11;
    g_SkyTileUV[3].corner[0].bytes.v = 12;
    g_SkyTileUV[3].corner[3].bytes.u = 31;
    g_SkyTileUV[3].corner[3].bytes.v = 32;
    g_EnvironmentColors.fields.slots[ENV_SKY_MIDDLE].cur.bytes.r = 10;
    g_EnvironmentColors.fields.slots[ENV_SKY_HORIZON].cur.bytes.r = 20;
    g_EnvironmentColors.fields.slots[ENV_SKY_TOP].cur.bytes.r = 30;

    DrawSkyBackground();

    horizonTile = (POLY_FT4 *)(void *)packets.bytes;
    CHECK(horizonTile->x0 == -32 && horizonTile->x1 == 32);
    CHECK(horizonTile->u0 == 11 && horizonTile->v0 == 12);
    CHECK(horizonTile->u3 == 31 && horizonTile->v3 == 32);
    CHECK(horizonTile->tpage == 0x18 && horizonTile->clut == 0x798E);
    CHECK(horizonTile->r0 == 0x80 && horizonTile->g0 == 0x80 &&
          horizonTile->b0 == 0x80);
    packetEnd = g_RenderState.packetCursor;
    gradient = (POLY_G4 *)(void *)(packetEnd - 4 * sizeof(POLY_G4));
    CHECK(gradient[0].r0 == 10 && gradient[0].r2 == 20);
    CHECK(gradient[1].r0 == 10 && gradient[1].r2 == 30);
    CHECK(gradient[2].r0 == 30 && gradient[2].r2 == 0);
    CHECK(gradient[2].b2 == 16 && gradient[2].b3 == 16);
    return 0;
}

static int TestInvalidSkyMapFallsBackToFirstTile(void) {
    PacketStorage packets;
    GameOrderingTableEntry orderingTable[GAME_FRAME_OT_LENGTH];
    POLY_FT4 *firstTile;

    PrepareFrame(&packets, orderingTable);
    g_CourseIndex = 2;
    g_SkyRowBase = INT32_MAX;
    g_SkyTileMap[1][4] = SKY_TILE_COUNT;
    g_SkyTileUV[0].corner[0].bytes.u = 17;
    g_SkyTileUV[0].corner[0].bytes.v = 23;

    DrawSkyBackground();

    firstTile = (POLY_FT4 *)(void *)packets.bytes;
    CHECK(firstTile->u0 == 17 && firstTile->v0 == 23);
    CHECK(g_RenderState.packetCursor ==
          packets.bytes + 32 * sizeof(POLY_FT4) + sizeof(POLY_G4));

    PrepareFrame(&packets, orderingTable);
    g_CourseIndex = 2;
    g_SkyTileMap[1][4] = -1;
    g_SkyTileUV[0].corner[0].bytes.u = 29;
    DrawSkyBackground();
    firstTile = (POLY_FT4 *)(void *)packets.bytes;
    CHECK(firstTile->u0 == 29);
    return 0;
}

static double NativeGridCoordinate(const GameSkyGridLayout *grid,
                                   double screenX, double screenY,
                                   int vertical) {
    double originX = grid->panelXFixed / 256.0;
    double originY = grid->panelYFixed / 256.0;
    double columnX = grid->columnStepX / 256.0;
    double columnY = grid->columnStepY / 256.0;
    double rowX = grid->rowStepX / 256.0;
    double rowY = grid->rowStepY / 256.0;
    double x = screenX - originX;
    double y = screenY - originY;
    double determinant = columnX * rowY - columnY * rowX;
    return vertical ? (columnX * y - columnY * x) / determinant
                    : (x * rowY - y * rowX) / determinant;
}

static int TestNativeGridMatchesClassicPackets(void) {
    static const struct {
        s32 y, pitch, yaw, roll;
    } cameras[] = {
        {6000, 0, 0, 0},
        {7120, -384, 511, 0},
        {4200, 640, 2047, 96},
        {9800, -900, 3584, -160},
    };
    PacketStorage packets;
    GameOrderingTableEntry orderingTable[GAME_FRAME_OT_LENGTH];

    for (size_t index = 0; index < sizeof(cameras) / sizeof(cameras[0]);
         index++) {
        GameSkyGridLayout grid;
        POLY_FT4 *tiles;
        PrepareFrame(&packets, orderingTable);
        g_CourseIndex = 2;
        g_RenderState.viewY = cameras[index].y;
        g_RenderState.viewAngleX = cameras[index].pitch;
        g_RenderState.viewAngleY = cameras[index].yaw;
        g_RenderState.viewAngleZ = cameras[index].roll;
        MeasureSkyGridLayout(cameras[index].y, cameras[index].pitch,
                             cameras[index].yaw, cameras[index].roll, 0, 0,
                             &grid);
        DrawSkyBackground();
        CHECK(s_publishedMirror == 0);
        CHECK(memcmp(&s_publishedGrid, &grid, sizeof(grid)) == 0);
        tiles = (POLY_FT4 *)(void *)packets.bytes;
        CHECK(tiles[0].x0 == grid.panelXFixed / 256);
        CHECK(tiles[0].y0 == grid.panelYFixed / 256);
        CHECK(tiles[0].x1 ==
              (grid.panelXFixed + grid.columnStepX) / 256);
        CHECK(tiles[0].y2 ==
              (grid.panelYFixed + grid.rowStepY) / 256);
        /* Emulate the native shader at every classic quad centre. Classic
         * resets each row to the origin and subtracts rowStep, so rows 1..3
         * occupy negative grid coordinates rather than extending down. */
        for (int row = 0; row < 4; row++) {
            for (int column = 0; column < 8; column++) {
                POLY_FT4 *tile = &tiles[row * 8 + column];
                double centerX =
                    (tile->x0 + tile->x1 + tile->x2 + tile->x3) / 4.0;
                double centerY =
                    (tile->y0 + tile->y1 + tile->y2 + tile->y3) / 4.0;
                CHECK(fabs(NativeGridCoordinate(
                               &grid, centerX, centerY, 0) -
                           (column + 0.5)) < 0.03);
                CHECK(fabs(NativeGridCoordinate(
                               &grid, centerX, centerY, 1) -
                           (0.5 - row)) < 0.03);
            }
        }
    }
    {
        GameSkyGridLayout before, after;
        double beforeTexture, afterTexture;
        MeasureSkyGridLayout(6000, 0, 0, 0, 0, 0, &before);
        MeasureSkyGridLayout(6000, 0, 64, 0, 0, 0, &after);
        beforeTexture = before.textureColumn +
            NativeGridCoordinate(&before, 160.0, 120.0, 0);
        afterTexture = after.textureColumn +
            NativeGridCoordinate(&after, 160.0, 120.0, 0);
        /* A positive camera yaw advances the original texture lookup. The
         * native background must therefore move the picture in the same
         * direction, not counter-scroll it. */
        CHECK(afterTexture > beforeTexture);
        CHECK(afterTexture - beforeTexture < 1.0);
    }
    return 0;
}

int main(void) {
    if (TestNearGroundCourseSkirt() != 0 ||
        TestFarGroundCourseSkirt() != 0 ||
        TestSkyGradientPaletteSlots() != 0 ||
        TestInvalidSkyMapFallsBackToFirstTile() != 0 ||
        TestNativeGridMatchesClassicPackets() != 0) {
        return 1;
    }
    puts("sky packet layout and both course-skirt paths are stable");
    return 0;
}
