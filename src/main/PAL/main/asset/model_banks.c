#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track.h"

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
    for (i = 0; i < count; i++) {
        if (!AssetPayloadOffsetIsValid(base->modelOffsets[i], payloadOffset,
                                       size)) {
            return 0;
        }
    }
    return 1;
}

s32 RegisterModelBank(const ModelBankHeader *base, size_t size, s32 index) {
    NativeModelBank *bank;
    u32 count;
    u32 i;

    if ((u32)index >= GAME_MODEL_BANK_LIMIT ||
        !IsValidModelBankAsset(base, size)) {
        return 0;
    }

    count = base->modelCount;
    bank = &g_ModelBanks[index];
    bank->modelCount = (s32)count;
    bank->table = ResolveConstAssetAddress(base, base->tableOffset);
    bank->normals = ResolveConstAssetAddress(base, base->normalsOffset);
    for (i = 0; i < count; i++) {
        bank->models[i] =
            ResolveConstAssetAddress(base, base->modelOffsets[i]);
    }
    return 1;
}

void SelectModelBank(s32 index) {
    const NativeModelBank *bank;

    if ((u32)index >= GAME_MODEL_BANK_LIMIT) return;
    bank = &g_ModelBanks[index];
    g_RenderState.modelTable1 = bank->table;
    g_RenderState.modelNormals = bank->normals;
    g_ModelBankCount = bank->modelCount;
    g_RenderState.modelModels = bank->models;
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
                                       size)) {
            return 0;
        }
    }
    return 1;
}

s32 RegisterCourseModels(const CourseModelAssetHeader *base, size_t size) {
    s32 count;
    s32 i;

    if (!IsValidCourseModelAsset(base, size)) return 0;

    count = base->modelCount;
    g_RenderState.courseBank = g_NativeCourseModels;
    g_CourseModelCount = count;
    for (i = 0; i < count; i++) {
        const CourseModelAssetEntry *entry = &base->models[i];

        g_NativeCourseModels[i].geometry =
            ResolveConstAssetAddress(base, entry->geometryOffset);
        g_NativeCourseModels[i].vertexCount = entry->vertexCount;
        g_NativeCourseModels[i].model =
            ResolveConstAssetAddress(base, entry->modelOffset);
    }
    return 1;
}
