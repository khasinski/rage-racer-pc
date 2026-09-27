#include "game/model_bank.h"
#include "game/model_stream.h"
#include "game/vector.h"
#include "game/asset_bounds.h"

s32 IsValidModelBankAsset(const ModelBankHeader *base, size_t size) {
    u32 count;
    u32 i;
    size_t payloadOffset;

    if (base == NULL || size < offsetof(ModelBankHeader, modelOffsets)) {
        return 0;
    }

    if (base->modelCount > GAME_MODEL_PER_BANK_LIMIT) return 0;
    count = base->modelCount;
    payloadOffset = offsetof(ModelBankHeader, modelOffsets) +
                    count * sizeof(base->modelOffsets[0]);
    if (size < payloadOffset ||
        !AssetPayloadOffsetIsValid(base->tableOffset, payloadOffset, size) ||
        !AssetPayloadOffsetIsValid(base->normalsOffset, payloadOffset,
                                   size)) {
        return 0;
    }
/* No element counts are stored: bound accesses by the remaining source
 * bytes. Logical section overlap is separate from memory safety. */
size_t vertices = (size - (size_t)base->tableOffset) / sizeof(SVec);
size_t normals = (size - (size_t)base->normalsOffset) / sizeof(SVec);
s32 vertexCount = (s32)(vertices > 65536 ? 65536 : vertices);
s32 normalCount = (s32)(normals > 65536 ? 65536 : normals);
for (i = 0; i < count; i++) {

        if (!AssetPayloadOffsetIsValid(base->modelOffsets[i], payloadOffset,
                                       size) ||
            !PrimitiveStreamIsValid((const u8 *)base, size,
                                    base->modelOffsets[i],
                                    ModelPrimitiveStride, vertexCount, normalCount)) {
            return 0;
        }
    }
    return 1;
}

s32 ReadModelBank(const ModelBankHeader *base, size_t size, NativeModelBank *bank) {
    if (bank == NULL || !IsValidModelBankAsset(base, size)) return 0;
    NativeModelBank parsed = {0};
    parsed.modelCount = (s32)base->modelCount;
    parsed.table = (const u8 *)base + base->tableOffset;
    parsed.normals = (const u8 *)base + base->normalsOffset;
    for (u32 i = 0; i < base->modelCount; i++)
        parsed.models[i] = (const u8 *)base + base->modelOffsets[i];
    *bank = parsed;
    return 1;
}

s32 IsValidCourseModelAsset(const CourseModelAssetHeader *base, size_t size) {
    s32 count;
    s32 i;
    size_t payloadOffset;

    if (base == NULL || size < offsetof(CourseModelAssetHeader, models)) {
        return 0;
    }
    if (base->modelCount < 0 ||
        base->modelCount > GAME_COURSE_MODEL_LIMIT) {
        return 0;
    }
    count = base->modelCount;
    payloadOffset = offsetof(CourseModelAssetHeader, models) +
                    (size_t)count * sizeof(base->models[0]);
    if (size < payloadOffset) {
        return 0;
    }
    for (i = 0; i < count; i++) {
        const CourseModelAssetEntry *entry = &base->models[i];

        if (entry->vertexCount < 0 ||
            !AssetPayloadOffsetIsValid(entry->geometryOffset, payloadOffset,
                                       size) ||
            !AssetPayloadOffsetIsValid(entry->modelOffset, payloadOffset,
                                       size) ||
            (size_t)entry->vertexCount >
                (size - (size_t)entry->geometryOffset) / sizeof(SVec) ||
            !PrimitiveStreamIsValid((const u8 *)base, size,
                                    entry->modelOffset,
                                    CoursePrimitiveStride,
                                    entry->vertexCount, -1)) {
            return 0;
        }
    }
    return 1;
}

s32 ReadCourseBank(const CourseModelAssetHeader *base, size_t size, CourseBank *bank) {
    if (!bank || !IsValidCourseModelAsset(base, size)) return 0;
    CourseBank view = {0};
    view.modelCount = base->modelCount;
    for (s32 i = 0; i < base->modelCount; ++i) {
        const CourseModelAssetEntry *entry = &base->models[i];
        view.models[i] = (NativeCourseModel){
            .geometry = (const u8 *)base + entry->geometryOffset,
            .vertexCount = entry->vertexCount,
            .model = (const u8 *)base + entry->modelOffset,
        };
    }
    *bank = view;
    return 1;
}
