#include "game/asset.h"
#include "content/track_pack.h"
#include "game/asset_internal.h"
#include "game/render.h"
#include "game/terrain_internal.h"
#include "game/track.h"

s32 IsValidTerrainCellAsset(const void *data,size_t size) {
    Terrain terrain;return TerrainOpen(data,size,&terrain);
}

s32 InstallTerrainCellData(const void *data, size_t size) {
    const u8 *cursor;
    const TerrainCellAssetHeader *header;
    s32 count;
    s32 i;

    if (!IsValidTerrainCellAsset(data, size)) return 0;

    cursor = data;
    cursor += TERRAIN_CELL_GRID_BYTES + CELL_VISIBILITY_TABLE_SIZE;
    header = (const TerrainCellAssetHeader *)cursor;
    count = header->cellCount;
    g_TerrainCellGrid = data;
    g_CellVisibilityTable =
        (const CellVisibilityRow *)((const u8 *)data +
                                    TERRAIN_CELL_GRID_BYTES);
    g_RenderState.geometry.cellTable = g_NativeTerrainCells;
    g_TerrainCellCount = count;
    g_RenderState.geometry.cellFaces = cursor + header->facesOffset;
    for (i = 0; i < count; i++) {
        g_NativeTerrainCells[i] = cursor + header->cellOffsets[i];
    }
    return 1;
}
