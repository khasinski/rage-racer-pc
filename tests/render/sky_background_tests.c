#include "game/environment.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/terrain_internal.h"
#include "game/track.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;
GameEnvironmentColors g_EnvironmentColors;
s16 g_SkyTileMap[SKY_TILE_MAP_ROWS][SKY_TILE_MAP_COLUMNS];
SkyTileUV g_SkyTileUV[SKY_TILE_COUNT];
s32 g_CourseIndex;
s32 g_MirrorMode;
s32 g_SkyRowBase;

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
    u8 *packetEnd;

    PrepareFrame(&packets, orderingTable);
    g_CourseIndex = 2;
    g_SkyRowBase = 0;
    g_EnvironmentColors.fields.slots[ENV_SKY_MIDDLE].cur.bytes.r = 10;
    g_EnvironmentColors.fields.slots[ENV_SKY_HORIZON].cur.bytes.r = 20;
    g_EnvironmentColors.fields.slots[ENV_SKY_TOP].cur.bytes.r = 30;

    DrawSkyBackground();

    packetEnd = g_RenderState.packetCursor;
    gradient = (POLY_G4 *)(void *)(packetEnd - 4 * sizeof(POLY_G4));
    CHECK(gradient[0].r0 == 10 && gradient[0].r2 == 20);
    CHECK(gradient[1].r0 == 10 && gradient[1].r2 == 30);
    CHECK(gradient[2].r0 == 30 && gradient[2].r2 == 0);
    CHECK(gradient[2].b2 == 16 && gradient[2].b3 == 16);
    return 0;
}

int main(void) {
    if (TestNearGroundCourseSkirt() != 0 ||
        TestFarGroundCourseSkirt() != 0 ||
        TestSkyGradientPaletteSlots() != 0) {
        return 1;
    }
    puts("sky packet layout and both course-skirt paths are stable");
    return 0;
}
