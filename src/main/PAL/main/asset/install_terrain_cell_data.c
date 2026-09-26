#include "game/asset.h"
#include <string.h>
#include "game/asset_internal.h"
#include "game/render.h"
#include "game/terrain_internal.h"
#include "game/track.h"

s32 InstallTerrainCellData(const void *data, size_t size) {
    TerrainBank bank;
    if (!ReadTerrainBank(data, size, &bank)) return 0;
    g_TerrainCellGrid = bank.grid;
    g_CellVisibilityTable = bank.visibility;
    g_RenderState.geometry.cellTable = g_NativeTerrainCells;
    g_TerrainCellCount = bank.cellCount;
    g_RenderState.geometry.cellFaces = bank.vertices;
    memcpy(g_NativeTerrainCells, bank.cells, sizeof(bank.cells));
    return 1;
}
