#include "render_world_snapshot.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    RAGE_RENDER_WORLD_SNAPSHOT_VERSION = 3,
    RAGE_RENDER_WORLD_SNAPSHOT_MAX_INSTANCES = 1000000,
};

static const unsigned char RAGE_RENDER_WORLD_SNAPSHOT_MAGIC[8] = {
    'R', 'A', 'G', 'E', 'F', 'R', 'M', '\0'};

static int WriteBytes(FILE *file, const void *bytes, size_t size) {
    return fwrite(bytes, 1, size, file) == size;
}

static int ReadBytes(FILE *file, void *bytes, size_t size) {
    return fread(bytes, 1, size, file) == size;
}

static int WriteU8(FILE *file, uint8_t value) {
    return WriteBytes(file, &value, sizeof(value));
}

static int ReadU8(FILE *file, uint8_t *value) {
    return ReadBytes(file, value, sizeof(*value));
}

static int WriteU32(FILE *file, uint32_t value) {
    const unsigned char bytes[4] = {
        (unsigned char)value,
        (unsigned char)(value >> 8),
        (unsigned char)(value >> 16),
        (unsigned char)(value >> 24),
    };
    return WriteBytes(file, bytes, sizeof(bytes));
}

static int ReadU32(FILE *file, uint32_t *value) {
    unsigned char bytes[4];
    if (!ReadBytes(file, bytes, sizeof(bytes))) return 0;
    *value = (uint32_t)bytes[0] |
             ((uint32_t)bytes[1] << 8) |
             ((uint32_t)bytes[2] << 16) |
             ((uint32_t)bytes[3] << 24);
    return 1;
}

static int WriteU64(FILE *file, uint64_t value) {
    return WriteU32(file, (uint32_t)value) &&
           WriteU32(file, (uint32_t)(value >> 32));
}

static int ReadU64(FILE *file, uint64_t *value) {
    uint32_t low, high;
    if (!ReadU32(file, &low) || !ReadU32(file, &high)) return 0;
    *value = (uint64_t)low | ((uint64_t)high << 32);
    return 1;
}

static int WriteFloat(FILE *file, float value) {
    uint32_t bits;
    if (sizeof(value) != sizeof(bits)) return 0;
    memcpy(&bits, &value, sizeof(bits));
    return WriteU32(file, bits);
}

static int ReadFloat(FILE *file, float *value) {
    uint32_t bits;
    if (sizeof(*value) != sizeof(bits) || !ReadU32(file, &bits)) return 0;
    memcpy(value, &bits, sizeof(bits));
    return 1;
}

static int WriteVec3(FILE *file, const RageRenderVec3 *value) {
    return WriteFloat(file, value->x) && WriteFloat(file, value->y) &&
           WriteFloat(file, value->z);
}

static int ReadVec3(FILE *file, RageRenderVec3 *value) {
    return ReadFloat(file, &value->x) && ReadFloat(file, &value->y) &&
           ReadFloat(file, &value->z);
}

static int WriteLight(FILE *file, const RageRenderDirectionalLight *value) {
    return WriteVec3(file, &value->direction) &&
           WriteVec3(file, &value->ambientColor) &&
           WriteVec3(file, &value->diffuseColor);
}

static int ReadLight(FILE *file, RageRenderDirectionalLight *value) {
    return ReadVec3(file, &value->direction) &&
           ReadVec3(file, &value->ambientColor) &&
           ReadVec3(file, &value->diffuseColor);
}

static int WriteTransform(FILE *file, const RageRenderTransform *value) {
    return WriteVec3(file, &value->position) &&
           WriteVec3(file, &value->rotation) &&
           WriteVec3(file, &value->scale) &&
           WriteFloat(file, value->orientation.x) &&
           WriteFloat(file, value->orientation.y) &&
           WriteFloat(file, value->orientation.z) &&
           WriteFloat(file, value->orientation.w) &&
           WriteU8(file, value->hasOrientation);
}

static int ReadTransform(FILE *file, RageRenderTransform *value) {
    return ReadVec3(file, &value->position) &&
           ReadVec3(file, &value->rotation) &&
           ReadVec3(file, &value->scale) &&
           ReadFloat(file, &value->orientation.x) &&
           ReadFloat(file, &value->orientation.y) &&
           ReadFloat(file, &value->orientation.z) &&
           ReadFloat(file, &value->orientation.w) &&
           ReadU8(file, &value->hasOrientation);
}

static int WriteCamera(FILE *file, const RageRenderCamera *value) {
    return WriteTransform(file, &value->transform) &&
           WriteFloat(file, value->verticalFovDegrees) &&
           WriteFloat(file, value->nearPlane) &&
           WriteFloat(file, value->farPlane) &&
           WriteVec3(file, &value->fogColor) &&
           WriteVec3(file, &value->skyTopColor) &&
           WriteVec3(file, &value->skyColor) &&
           WriteVec3(file, &value->skyHorizonColor) &&
           WriteVec3(file, &value->skyBottomColor) &&
           WriteU32(file, value->skyAssetKey) &&
           WriteFloat(file, value->fogNear) &&
           WriteFloat(file, value->fogFar);
}

static int ReadCamera(FILE *file, RageRenderCamera *value, uint32_t version) {
    if (!ReadTransform(file, &value->transform) ||
        !ReadFloat(file, &value->verticalFovDegrees) ||
        !ReadFloat(file, &value->nearPlane) ||
        !ReadFloat(file, &value->farPlane) ||
        !ReadVec3(file, &value->fogColor)) return 0;
    if (version >= 2) {
        return ReadVec3(file, &value->skyTopColor) &&
               ReadVec3(file, &value->skyColor) &&
               ReadVec3(file, &value->skyHorizonColor) &&
               ReadVec3(file, &value->skyBottomColor) &&
               ReadU32(file, &value->skyAssetKey) &&
               ReadFloat(file, &value->fogNear) &&
               ReadFloat(file, &value->fogFar);
    }
    if (!ReadVec3(file, &value->skyColor)) return 0;
    /* Version 1 recorded one flat backdrop colour. Preserve that exact
     * appearance when old deterministic captures are replayed. */
    value->skyTopColor = value->skyColor;
    value->skyHorizonColor = value->skyColor;
    value->skyBottomColor = value->skyColor;
    value->skyAssetKey = UINT32_MAX;
    return ReadFloat(file, &value->fogNear) &&
           ReadFloat(file, &value->fogFar);
}

static int WriteInstance(FILE *file,
                         const RageRenderMeshInstance *value) {
    return WriteU32(file, value->entity) &&
           WriteU32(file, value->mesh) &&
           WriteU32(file, (uint32_t)value->assetSet) &&
           WriteU32(file, value->assetKey) &&
           WriteU32(file, value->material) &&
           WriteU8(file, value->component) &&
           WriteU8(file, value->materialVariant) &&
           WriteU8(file, value->hasCarPaint) &&
           WriteU8(file, value->carPaintColor1) &&
           WriteU8(file, value->carPaintColor2) &&
           WriteU8(file, value->textureScrollU) &&
           WriteVec3(file, &value->environmentLight) &&
           WriteFloat(file, value->depthBias) &&
           WriteTransform(file, &value->transform) &&
           WriteTransform(file, &value->previousTransform) &&
           WriteU32(file, value->flags) &&
           WriteU32(file, (uint32_t)value->pass);
}

static int ReadInstance(FILE *file, RageRenderMeshInstance *value) {
    uint32_t assetSet, pass;
    if (!ReadU32(file, &value->entity) ||
        !ReadU32(file, &value->mesh) ||
        !ReadU32(file, &assetSet) ||
        !ReadU32(file, &value->assetKey) ||
        !ReadU32(file, &value->material) ||
        !ReadU8(file, &value->component) ||
        !ReadU8(file, &value->materialVariant) ||
        !ReadU8(file, &value->hasCarPaint) ||
        !ReadU8(file, &value->carPaintColor1) ||
        !ReadU8(file, &value->carPaintColor2) ||
        !ReadU8(file, &value->textureScrollU) ||
        !ReadVec3(file, &value->environmentLight) ||
        !ReadFloat(file, &value->depthBias) ||
        !ReadTransform(file, &value->transform) ||
        !ReadTransform(file, &value->previousTransform) ||
        !ReadU32(file, &value->flags) ||
        !ReadU32(file, &pass)) return 0;
    if (assetSet > RAGE_RENDER_ASSET_TRACK_MODEL_BANK_2 ||
        pass > RAGE_RENDER_PASS_MIRROR) return 0;
    value->assetSet = (RageRenderAssetSet)assetSet;
    value->pass = (RageRenderPass)pass;
    return 1;
}

int RageRenderWorldSnapshotWrite(const char *path,
                                 const RageRenderWorld *world) {
    FILE *file;
    uint32_t instance;
    int ok;
    if (path == NULL || world == NULL ||
        (world->instanceCount != 0 && world->instances == NULL)) return 0;
    file = fopen(path, "wb");
    if (file == NULL) return 0;
    ok = WriteBytes(file, RAGE_RENDER_WORLD_SNAPSHOT_MAGIC,
                    sizeof(RAGE_RENDER_WORLD_SNAPSHOT_MAGIC)) &&
         WriteU32(file, RAGE_RENDER_WORLD_SNAPSHOT_VERSION) &&
         WriteU64(file, world->frame) &&
         WriteLight(file, &world->light) &&
         WriteCamera(file, &world->camera) &&
         WriteCamera(file, &world->previousCamera) &&
         WriteU8(file, world->hasCamera) &&
         WriteCamera(file, &world->mirrorCamera) &&
         WriteCamera(file, &world->previousMirrorCamera) &&
         WriteFloat(file, world->mirrorPanelY) &&
         WriteFloat(file, world->previousMirrorPanelY) &&
         WriteU8(file, world->hasMirrorCamera) &&
         WriteU8(file, world->mirrorActive) &&
         WriteU32(file, world->instanceCount) &&
         WriteU32(file, world->overflowCount);
    for (instance = 0; ok && instance < world->instanceCount; instance++)
        ok = WriteInstance(file, &world->instances[instance]);
    if (fclose(file) != 0) ok = 0;
    if (!ok) remove(path);
    return ok;
}

int RageRenderWorldSnapshotRead(const char *path,
                                RageRenderWorldSnapshot *snapshot) {
    unsigned char magic[sizeof(RAGE_RENDER_WORLD_SNAPSHOT_MAGIC)];
    uint32_t version, instance, count;
    FILE *file;
    int ok;
    if (path == NULL || snapshot == NULL) return 0;
    memset(snapshot, 0, sizeof(*snapshot));
    file = fopen(path, "rb");
    if (file == NULL) return 0;
    ok = ReadBytes(file, magic, sizeof(magic)) &&
         memcmp(magic, RAGE_RENDER_WORLD_SNAPSHOT_MAGIC, sizeof(magic)) == 0 &&
         ReadU32(file, &version) &&
         (version >= 1 && version <= RAGE_RENDER_WORLD_SNAPSHOT_VERSION) &&
         ReadU64(file, &snapshot->world.frame);
    if (ok && version >= 3)
        ok = ReadLight(file, &snapshot->world.light);
    else if (ok)
        RageRenderDirectionalLightDefault(&snapshot->world.light);
    ok = ok &&
         ReadCamera(file, &snapshot->world.camera, version) &&
         ReadCamera(file, &snapshot->world.previousCamera, version) &&
         ReadU8(file, &snapshot->world.hasCamera) &&
         ReadCamera(file, &snapshot->world.mirrorCamera, version) &&
         ReadCamera(file, &snapshot->world.previousMirrorCamera, version) &&
         ReadFloat(file, &snapshot->world.mirrorPanelY) &&
         ReadFloat(file, &snapshot->world.previousMirrorPanelY) &&
         ReadU8(file, &snapshot->world.hasMirrorCamera) &&
         ReadU8(file, &snapshot->world.mirrorActive) &&
         ReadU32(file, &count) &&
         count <= RAGE_RENDER_WORLD_SNAPSHOT_MAX_INSTANCES &&
         ReadU32(file, &snapshot->world.overflowCount);
    if (ok && count != 0) {
        snapshot->instances = calloc(count, sizeof(*snapshot->instances));
        ok = snapshot->instances != NULL;
    }
    for (instance = 0; ok && instance < count; instance++)
        ok = ReadInstance(file, &snapshot->instances[instance]);
    if (ok) {
        unsigned char trailing;
        ok = fread(&trailing, 1, 1, file) == 0 && feof(file);
    }
    fclose(file);
    if (!ok) {
        RageRenderWorldSnapshotRelease(snapshot);
        return 0;
    }
    snapshot->world.instances = snapshot->instances;
    snapshot->world.instanceCapacity = count;
    snapshot->world.instanceCount = count;
    return 1;
}

void RageRenderWorldSnapshotRelease(RageRenderWorldSnapshot *snapshot) {
    if (snapshot == NULL) return;
    free(snapshot->instances);
    memset(snapshot, 0, sizeof(*snapshot));
}
