#include <stddef.h>

#include "render_scene.h"

static uint64_t HashBytes(uint64_t hash, const void *data, size_t size) {
    const uint8_t *bytes = (const uint8_t *)data;
    size_t i;
    for (i = 0; i < size; i++) {
        hash ^= bytes[i];
        hash *= 0x100000001B3ull;
    }
    return hash;
}

uint64_t RageRenderSceneHash(const RageRenderScene *scene) {
    uint64_t hash = 0xCBF29CE484222325ull;
    int i;
    hash = HashBytes(hash, &scene->sceneId, sizeof(scene->sceneId));
    hash = HashBytes(hash, &scene->sceneTimer, sizeof(scene->sceneTimer));
    hash = HashBytes(hash, &scene->viewMatrix, sizeof(scene->viewMatrix));
    hash = HashBytes(hash, scene->viewPosition, sizeof(scene->viewPosition));
    for (i = 0; i < scene->drawCount; i++) {
        RageRenderModelDraw draw = scene->draws[i];
        /* Native pointers vary with ASLR and are identity, not scene data. */
        draw.bankId = 0;
        hash = HashBytes(hash, &draw, sizeof(draw));
    }
    for (i = 0; i < scene->terrainCount; i++)
        hash = HashBytes(hash, &scene->terrain[i], sizeof(scene->terrain[i]));
    for (i = 0; i < scene->packetCount; i++)
        hash = HashBytes(hash, &scene->packets[i], sizeof(scene->packets[i]));
    for (i = 0; i < scene->faceCount; i++)
        hash = HashBytes(hash, &scene->faces[i], sizeof(scene->faces[i]));
    return hash;
}
