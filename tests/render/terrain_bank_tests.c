#include "game/terrain_bank.h"
#include "game/vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)

int main(void) {
    const size_t prefix = TERRAIN_CELL_GRID_BYTES + CELL_VISIBILITY_TABLE_SIZE;
    const size_t size = prefix + 96;
    u8 *data = calloc(1, size);
    CHECK(data != NULL);
    TerrainCellAssetHeader *header = (void *)(data + prefix);
    *header = (TerrainCellAssetHeader){.cellCount = 1, .facesOffset = 16, .cellOffsets = {48}};
    u16 *stream = (void *)(data + prefix + 48);
    stream[1] = 1;
    for (u16 corner = 0; corner < 4; ++corner) stream[2 + corner] = corner;
    TerrainBank bank;
    CHECK(ReadTerrainBank(data, size, &bank));
    CHECK(bank.cellCount == 1 && bank.cells[0] == stream);
    CHECK(bank.vertices == data + prefix + 16 && bank.grid == (const u16 *)data);
    CHECK(bank.visibility == (const void *)(data + TERRAIN_CELL_GRID_BYTES));
    CHECK(bank.cells[1] == NULL);
    const TerrainBank saved = bank;
    const u16 capacity = (96 - 16) / sizeof(SVec);
    for (unsigned corner = 0; corner < 4; ++corner) {
        stream[2 + corner] = capacity;
        CHECK(!ReadTerrainBank(data, size, &bank));
        CHECK(memcmp(&bank, &saved, sizeof(bank)) == 0);
        stream[2 + corner] = (u16)corner;
    }
    CHECK(!ReadTerrainBank(data, prefix + 84, &bank)); /* Missing terminator. */
    CHECK(!ReadTerrainBank(data + 1, size - 1, &bank));
    CHECK(!ReadTerrainBank(NULL, size, &bank));
    CHECK(!ReadTerrainBank(data, size, NULL));
    CHECK(memcmp(&bank, &saved, sizeof(bank)) == 0);
    ((u16 *)data)[0] = 1; /* Grid refers to a nonexistent cell. */
    CHECK(!ReadTerrainBank(data, size, &bank));
    CHECK(memcmp(&bank, &saved, sizeof(bank)) == 0);
    ((u16 *)data)[0] = TERRAIN_MISSING_CELL_INDEX;
    CHECK(ReadTerrainBank(data, size, &bank));
    u8 *other = malloc(size);
    CHECK(other != NULL);
    memcpy(other, data, size);
    TerrainBank second;
    CHECK(ReadTerrainBank(other, size, &second));
    CHECK(second.cells[0] != bank.cells[0] && second.vertices != bank.vertices);
    header->cellOffsets[0] = -1;
    CHECK(!ReadTerrainBank(data, size, &bank));
    CHECK(bank.cells[0] == saved.cells[0]);
    free(data);
    CHECK(second.cellCount == 1 && *(const u16 *)second.cells[0] == 0);
    free(other);
    return 0;
}
