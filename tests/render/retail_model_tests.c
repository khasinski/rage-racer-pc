#include "game/race_data.h"
#include "game/asset_index.h"
#include "native_mesh_writer.h"
#include "native_texture.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: retail_model_tests <CUE or Track 01 BIN>\n");
        return 2;
    }
    RaceData *archive = LoadRaceDisc(argv[1]);
    if (!archive) return 3;
    CarModelData *models[CAR_MODEL_VARIANT_COUNT] = {0};
    RageImportedMeshEntry entries[CAR_MODEL_VARIANT_COUNT] = {0};
    uint32_t count = 0;
    int result = 1;
    uint8_t *pixels = malloc(256u * 256u * 4u);
    if (!pixels) goto cleanup;
    for (unsigned variant = 0; variant < CAR_MODEL_VARIANT_COUNT; ++variant) {
        models[variant] = CopyRaceCarModel(archive, (s32)variant);
        if (!models[variant]) {
            fprintf(stderr, "invalid source model %u\n", variant);
            goto cleanup;
        }
    }
    FreeRaceData(archive);
    archive = NULL;
    if (!ImportPrepareCars(entries, &count, CAR_MODEL_VARIANT_COUNT, models) ||
        count != CAR_MODEL_VARIANT_COUNT) {
        fprintf(stderr, "could not prepare complete car model cache\n");
        goto cleanup;
    }
    for (unsigned variant = 0; variant < CAR_MODEL_VARIANT_COUNT; ++variant) {
        FreeCarModelData(models[variant]);
        models[variant] = NULL;
    }
    for (unsigned variant = 0; variant < count; ++variant) {
        const RageImportedMeshEntry *entry = &entries[variant];
        if (entry->cached.assetKey != (u32)CarVariantAssetIndex(ASSET_CAR_1ST_BASE, (s32)variant) ||
            !entry->carSource || !entry->carSource->image ||
            !entry->cached.mesh.vertexCount || !entry->cached.mesh.indexCount) goto cleanup;
        for (u32 i = 0; i < entry->cached.mesh.vertexCount; ++i) {
            RageRuntimeVertex vertex;
            if (!RuntimeMeshVertex(&entry->cached.mesh, i, &vertex)) goto cleanup;
            for (unsigned axis = 0; axis < 3; ++axis)
                if (!isfinite(vertex.position[axis]) || !isfinite(vertex.normal[axis])) goto cleanup;
        }
        for (u32 i = 0; i < entry->cached.mesh.indexCount; ++i) {
            u32 index;
            if (!RuntimeMeshIndex(&entry->cached.mesh, i, &index) ||
                index >= entry->cached.mesh.vertexCount) goto cleanup;
        }
        TextureImage images[3];
        if (!CarTextureImages(entry->carSource, images)) goto cleanup;
        for (unsigned material = 0; material < entry->materialCount; ++material) {
            const RageImportedTextureKey *texture = &entry->materials[material];
            if (!DecodeTexture(texture, texture->clut, images, pixels,
                               256u * 256u * 4u, NULL, 0)) goto cleanup;
            unsigned opaque = 0;
            for (unsigned pixel = 0; pixel < 256u * 256u; ++pixel)
                opaque += pixels[pixel * 4 + 3] != 0;
            if (!opaque) {
                fprintf(stderr, "model %u material %u: transparent source tpage=%u clut=%u\n",
                       variant, material, texture->tpage, texture->clut);
                goto cleanup;
            }
        }
        printf("model %u: meshes=%u vertices=%u indices=%u materials=%u\n",
               variant, entry->cached.mesh.meshCount, entry->cached.mesh.vertexCount,
               entry->cached.mesh.indexCount, entry->materialCount);
    }
    result = 0;
cleanup:
    free(pixels);
    FreeRaceData(archive);
    for (unsigned variant = 0; variant < CAR_MODEL_VARIANT_COUNT; ++variant)
        FreeCarModelData(models[variant]);
    for (unsigned i = 0; i < count; ++i) ImportReleaseEntry(&entries[i]);
    return result;
}
