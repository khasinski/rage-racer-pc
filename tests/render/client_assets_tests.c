#include "client_race.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void) {
    ClientRace *first = calloc(1, sizeof(*first)), *second = calloc(1, sizeof(*second));
    u8 *rgba = malloc(256 * 256 * 4), *saved = malloc(256 * 256 * 4);
    CHECK(first && second && rgba && saved);
    first->pixels = calloc(1, sizeof(*first->pixels));
    second->pixels = calloc(1, sizeof(*second->pixels));
    CHECK(first->pixels && second->pixels);
    const u8 geometry = 0;
    RageImportedTextureKey texture = {.tpage = 281};
    first->courseMesh = (RageImportedMeshEntry){.source = RENDER_ASSET_OWNED,
        .materials = &texture, .materialCount = 1};
    first->courseMesh.cached.assetKey = 88;
    first->courseMesh.cached.assetSet = RAGE_RENDER_ASSET_COURSE;
    first->courseMesh.cached.mesh.bytes = &geometry;
    first->courseMesh.cached.mesh.meshCount = 1;
    second->courseMesh = first->courseMesh;
    RenderMeshInstance instance = {.assetKey = 88, .assetSet = RAGE_RENDER_ASSET_COURSE,
                                   .assetSource = RENDER_ASSET_OWNED};
    CHECK(FindClientMesh(first, &instance) == &first->courseMesh);
    CHECK(FindClientMesh(second, &instance) == &second->courseMesh);
    first->pixels->pages[0][256 * 1024 + 576] = 0x001f;
    second->pixels->pages[0][256 * 1024 + 576] = 0x03e0;
    CHECK(DecodeClientMaterial(first, &instance, 0, 0, NULL, rgba, 256 * 256 * 4));
    CHECK(rgba[0] == 255 && rgba[1] == 0);
    CHECK(DecodeClientMaterial(second, &instance, 0, 0, NULL, rgba, 256 * 256 * 4));
    CHECK(rgba[0] == 0 && rgba[1] == 255);
first->pixels->pages[1][256 * 1024 + 576] = 0x7c00;
instance.materialVariant = 4;
CHECK(DecodeClientMaterial(first, &instance, 0, 0, NULL, rgba, 256 * 256 * 4));
CHECK(rgba[0] == 0 && rgba[2] == 255);
instance.materialVariant = 0;
texture = (RageImportedTextureKey){.tpage = 0, .clut = 0x798e};
first->pixels->pages[0][0] = 0x1111;
const u16 palette[16] = {0, 0x7c1f};
CHECK(DecodeClientMaterial(first, &instance, 0, 0, palette, rgba, 256 * 256 * 4));
CHECK(rgba[0] == 255 && rgba[2] == 255);
CHECK(first->pixels->pages[0][486 * 1024 + 225] == 0);
    memcpy(saved, rgba, 256 * 256 * 4);
    instance.assetSource = RENDER_ASSET_DEFAULT;
    CHECK(!FindClientMesh(first, &instance));
    CHECK(!DecodeClientMaterial(first, &instance, 0, 0, NULL, rgba, 256 * 256 * 4));
    instance.assetSource = RENDER_ASSET_OWNED;
    instance.mesh = 1;
    CHECK(!FindClientMesh(first, &instance));
    instance.mesh = 0;
    instance.assetKey = 90;
    CHECK(!FindClientMesh(first, &instance));
    instance.assetKey = 88;
    CHECK(!DecodeClientMaterial(first, &instance, 1, 0, NULL, rgba, 256 * 256 * 4));
    CHECK(!DecodeClientMaterial(first, &instance, 0, 2, NULL, rgba, 256 * 256 * 4));
    CHECK(!DecodeClientMaterial(first, &instance, 0, 0, NULL, rgba, 1));
    CHECK(!FindClientMesh(NULL, &instance) && !FindClientMesh(first, NULL));
    CHECK(memcmp(saved, rgba, 256 * 256 * 4) == 0);
    free(first->pixels); free(second->pixels); free(first); free(second); free(rgba); free(saved);
    return 0;
}
