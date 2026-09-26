#include "game/car_model_data.h"
#include "game/race_data.h"
#include "game/asset_index.h"
#include "native_mesh_writer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)

static int TestArchiveMesh(void) {
const u32 shared[] = {0, 24, 0, 0, 16, (48u << 16) | 640u,
    0x00010002, 0x001f001f, 24, 0, 0, 16, (480u << 16) | 16u,
    0x00010002, 0x03e003e0, 0};
const s32 modelSize = 80;
    size_t size = sizeof(SerializedCarModelAssetHeader) + modelSize + sizeof(CarImageData);
    u8 *bytes = calloc(1, size + sizeof(shared));
    CHECK(bytes != NULL);
    SerializedCarModelAssetHeader header = {.modelOffset = sizeof(header),
        .imageOffset = sizeof(header) + modelSize};
    memcpy(header.metadata + 0x18, &modelSize, sizeof(modelSize));
    memcpy(bytes, &header, sizeof(header));
    const s32 bankHeader[] = {1, 16, 48, 52};
    memcpy(bytes + header.modelOffset, bankHeader, sizeof(bankHeader));
    const SVec vertices[4] = {{.vx = 17, .vy = -18, .vz = 19},
        {.vx = 27}, {.vx = 37}, {.vx = 47}};
    memcpy(bytes + header.modelOffset + 16, vertices, sizeof(vertices));
    const u16 stream[] = {0, 1, 0, 1, 2, 3, 0x2211, 0x0033, 0, 0, 0, 0};
    memcpy(bytes + header.modelOffset + 52, stream, sizeof(stream));
    memcpy(bytes + size, shared, sizeof(shared));
    RaceData archive = {.data = bytes, .size = size + sizeof(shared)};
    archive.entries[ASSET_BOOT_CAR_SCREEN] = (RageArchiveIndexEntry){.byteOffset = (u32)size, .size = sizeof(shared)};
    archive.entries[CarVariantAssetIndex(ASSET_CAR_1ST_BASE, 7)] =
        (RageArchiveIndexEntry){.size = (u32)size};
    CarModelData *owned = CopyRaceCarModel(&archive, 7);
    CHECK(owned != NULL);
CHECK(owned->hasSharedImage && owned->sharedImage[48 * 64] == 0x001f);
CHECK(owned->logoPalette[0] == 0x03e0);
CHECK(!ReadCarModelImages(owned, shared, sizeof(shared) - 1));
CHECK(owned->sharedImage[48 * 64] == 0x001f && owned->logoPalette[0] == 0x03e0);
archive.entries[ASSET_BOOT_CAR_SCREEN].size = 0;
CHECK(CopyRaceCarModel(&archive, 7) == NULL);

    memset(bytes, 0, size); free(bytes);
    RenderMeshInstance identity = {.assetSet = RAGE_RENDER_ASSET_MODEL_BANK,
        .assetKey = (u32)CarVariantAssetIndex(ASSET_CAR_1ST_BASE, 7)};
    RageImportedMeshEntry entry = {0};
    CHECK(ImportBuildBankMesh(&identity, &owned->bank, &entry));
    RageImportedMeshEntry cache[3] = {0};
    uint32_t count = 0;
    CarModelData *models[CAR_MODEL_VARIANT_COUNT] = {0};
    models[7] = owned;
    CHECK(ImportPrepareCars(cache, &count, 3, models));
    CHECK(count == 1 && cache[0].carSource != owned);
    const void *cachedBytes = cache[0].cached.mesh.bytes;
CHECK(cache[0].carSource->hasSharedImage);
CHECK(cache[0].carSource->sharedImage[48 * 64] == 0x001f);
CHECK(cache[0].carSource->logoPalette[0] == 0x03e0);
owned->sharedImage[0] = 1;
CHECK(!ImportPrepareCars(cache, &count, 3, models));
CHECK(count == 1 && cache[0].cached.mesh.bytes == cachedBytes);
owned->sharedImage[0] = 0;

    RageImportedMeshEntry mixed[2] = {0};
    mixed[0].cached.assetKey = identity.assetKey;
    mixed[0].cached.assetSet = identity.assetSet;
    uint32_t mixedCount = 1;
    CHECK(ImportPrepareCars(mixed, &mixedCount, 2, models));
    CHECK(mixedCount == 2 && mixed[0].carSource == NULL && mixed[1].carSource != NULL);
    CHECK(mixed[0].cached.assetKey == identity.assetKey);
    CHECK(ImportFindMesh(mixed, mixedCount, identity.assetKey, identity.assetSet, RENDER_ASSET_DEFAULT) == &mixed[0]);
    CHECK(ImportFindMesh(mixed, mixedCount, identity.assetKey, identity.assetSet, RENDER_ASSET_OWNED) == &mixed[1]);
    CHECK(!ImportFindMesh(mixed, mixedCount, identity.assetKey, identity.assetSet, RENDER_ASSET_SOURCE_COUNT));
    CHECK(!ImportFindMesh(NULL, mixedCount, identity.assetKey, identity.assetSet, RENDER_ASSET_OWNED));
    /* Owned track meshes have no carSource; ownership is still unambiguous. */
    RageImportedMeshEntry track = {.source = RENDER_ASSET_OWNED};
    track.cached.assetKey = 88;
    track.cached.assetSet = RAGE_RENDER_ASSET_COURSE;
    CHECK(ImportFindMesh(&track, 1, 88, RAGE_RENDER_ASSET_COURSE, RENDER_ASSET_OWNED) == &track);
    CHECK(!ImportFindMesh(&track, 1, 88, RAGE_RENDER_ASSET_COURSE, RENDER_ASSET_DEFAULT));

    ImportReleaseEntry(&mixed[1]);

    CHECK(ImportPrepareCars(cache, &count, 3, models));
    CHECK(count == 1 && cache[0].cached.mesh.bytes == cachedBytes);
    CarModelData *empty = CopyCarModelData(owned->bytes, owned->size);
    CHECK(empty != NULL);
    /* Structurally valid source with an empty model cannot produce a mesh. */
    memset(empty->bytes + header.modelOffset + 52, 0, sizeof(stream));
    models[8] = owned; models[9] = empty;
    CHECK(!ImportPrepareCars(cache, &count, 3, models));
    CHECK(count == 1 && cache[0].cached.mesh.bytes == cachedBytes);
    CHECK(cache[1].carSource == NULL && cache[1].cached.mesh.bytes == NULL);
    models[9] = owned;
    CHECK(!ImportPrepareCars(cache, &count, 2, models));
    CHECK(count == 1 && cache[0].cached.mesh.bytes == cachedBytes);
    CHECK(cache[1].carSource == NULL && cache[1].materials == NULL);
    models[9] = NULL;

    CHECK(!ImportPrepareCars(cache, &count, 1, models));
    CHECK(count == 1 && cache[0].cached.mesh.bytes == cachedBytes);
    CHECK(ImportPrepareCars(cache, &count, 3, models));
    CHECK(count == 2 && cache[1].carSource != owned);
    /* Same key, different canonical bytes: preserve published resources. */
    models[7] = empty;
    CHECK(!ImportPrepareCars(cache, &count, 3, models));
    CHECK(count == 2 && cache[0].cached.mesh.bytes == cachedBytes);
    FreeCarModelData(empty);
    FreeCarModelData(owned);
    for (uint32_t i = 0; i < count; ++i) {
        RageRuntimeVertex cachedVertex;
        CHECK(RuntimeMeshVertex(&cache[i].cached.mesh, 0, &cachedVertex));
        CHECK(cachedVertex.position[0] == 17);
        CHECK(cache[i].carSource->image != NULL);
        CHECK(cache[i].carSource->sharedImage[48 * 64] == 0x001f);
        CHECK(cache[i].carSource->logoPalette[0] == 0x03e0);
        ImportReleaseEntry(&cache[i]);
        CHECK(cache[i].cached.mesh.bytes == NULL && cache[i].carSource == NULL);
    }

    RageRuntimeVertex vertex;
    CHECK(RuntimeMeshVertex(&entry.cached.mesh, 0, &vertex));
    CHECK(vertex.position[0] == 17 && vertex.position[1] == 18 && vertex.position[2] == -19);
    CHECK(vertex.color[0] == 17 && vertex.color[1] == 34 && vertex.color[2] == 51);
    CHECK(entry.cached.mesh.vertexCount == 4 && entry.cached.mesh.indexCount == 6);
    RuntimeCachedMeshRelease(&entry.cached); free(entry.materials);
    return 0;
}

int main(void) {
    CHECK(TestArchiveMesh() == 0);
    const s32 modelSize = 32;
    const size_t size = sizeof(SerializedCarModelAssetHeader) + modelSize + sizeof(CarImageData);
    u8 *storage = calloc(1, size + 9);
    CHECK(storage != NULL);
    u8 *bytes = storage + 1; /* source need not be aligned */
    SerializedCarModelAssetHeader header = {.modelOffset = sizeof(header),
        .imageOffset = sizeof(header) + modelSize};
    CarShape shape = {10, -20, 30, 40};
    memcpy(header.metadata, &shape, sizeof(shape));
    memcpy(header.metadata + 0x18, &modelSize, sizeof(modelSize));
    memcpy(bytes, &header, sizeof(header));
    const s32 bank[] = {1, 20, 24, 28, 0, 0, 0, 0};
    memcpy(bytes + header.modelOffset, bank, sizeof(bank));
    memset(bytes + header.imageOffset, 0x7D, sizeof(CarImageData));
    CarModelData *first = CopyCarModelData(bytes, size);
    CarModelData *second = CopyCarModelData(bytes, size);
    CHECK(first != NULL && second != NULL);
    CHECK(memcmp(&first->shape, &shape, sizeof(shape)) == 0);
    CHECK(first->bank.modelCount == 1);
    CHECK(first->bank.models[0] == first->bytes + header.modelOffset + 28);
    CHECK(first->image == (const CarImageData *)(const void *)(first->bytes + header.imageOffset));
    CHECK(first->bank.models[0] != second->bank.models[0] && first->image != second->image);
    CHECK(first->size == size && memcmp(first->bytes, bytes, size) == 0);
    RaceData archive = {.data = bytes, .size = size + 8};
    archive.entries[ASSET_BOOT_CAR_SCREEN] = (RageArchiveIndexEntry){.byteOffset = (u32)size, .size = 8};
    archive.entries[CarVariantAssetIndex(ASSET_CAR_1ST_BASE, 31)] =
        (RageArchiveIndexEntry){.size = (u32)size};
    CarModelData *selected = CopyRaceCarModel(&archive, 31);
    CHECK(selected != NULL && selected->bank.modelCount == 1);
    CHECK(CopyRaceCarModel(&archive, 0) == NULL);
    CHECK(CopyRaceCarModel(&archive, -1) == NULL);
    CHECK(CopyRaceCarModel(&archive, CAR_MODEL_VARIANT_COUNT) == NULL);
    CHECK(CopyRaceCarModel(NULL, 31) == NULL);
    for (size_t truncated = 0; truncated < size; truncated++)
        CHECK(CopyCarModelData(bytes, truncated) == NULL);
    CHECK(CopyCarModelData(NULL, size) == NULL);
    header.modelOffset++;
    memcpy(bytes, &header, sizeof(header));
    CHECK(CopyCarModelData(bytes, size) == NULL);
    header.modelOffset--;
    header.imageOffset++;
    memcpy(bytes, &header, sizeof(header));
    CHECK(CopyCarModelData(bytes, size) == NULL);
    header.imageOffset--;
    s32 negative = -1;
    memcpy(header.metadata + 0x18, &negative, sizeof(negative));
    memcpy(bytes, &header, sizeof(header));
    CHECK(CopyCarModelData(bytes, size) == NULL);
    memcpy(header.metadata + 0x18, &modelSize, sizeof(modelSize));
    memcpy(bytes, &header, sizeof(header));
    s32 invalidCount = -1;
    memcpy(bytes + header.modelOffset, &invalidCount, sizeof(invalidCount));
    CHECK(CopyCarModelData(bytes, size) == NULL);
    memset(storage, 0, size + 1);
    free(storage);
    CHECK(first->shape.offsetX == 10 && second->shape.horizon == 40);
    CHECK(first->image->reserved[0] == 0x7D && selected->image->reserved[0] == 0x7D);
    CHECK(*(const u32 *)first->bank.models[0] == 0);
    FreeCarModelData(first);
    FreeCarModelData(second);
    FreeCarModelData(selected);
    FreeCarModelData(NULL);
    return 0;
}
