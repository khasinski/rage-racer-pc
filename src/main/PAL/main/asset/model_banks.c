#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/model_stream.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track.h"

typedef s32 (*PrimitiveStride)(s32 primitive);

static u16 ReadAssetU16(const u8 *bytes) {
    return (u16)(bytes[0] | (u16)bytes[1] << 8);
}

static s32 PrimitiveStreamIsValid(const u8 *base, size_t size, s32 offset,
                                  PrimitiveStride primitiveStride,
                                  s32 vertexCount) {
    size_t cursor = (size_t)offset;

    while (cursor <= size && size - cursor >= sizeof(u32)) {
        const u16 primitive = ReadAssetU16(&base[cursor]);
        const u16 count = ReadAssetU16(&base[cursor + 2]);
        const s32 stride = primitiveStride(primitive);
        u16 record;

        cursor += sizeof(u32);
        if (count == 0) return 1;
        if (stride == 0 || count > (size - cursor) / (size_t)stride) {
            return 0;
        }
        if (vertexCount >= 0) {
            for (record = 0; record < count; record++) {
                const u8 *face = &base[cursor + (size_t)record * stride];
                s32 corner;

                for (corner = 0; corner < 4; corner++) {
                    if (ReadAssetU16(&face[corner * sizeof(u16)]) >=
                        vertexCount) {
                        return 0;
                    }
                }
            }
        }
        cursor += (size_t)count * (size_t)stride;
    }
    return 0;
}

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
                                       size) ||
            !PrimitiveStreamIsValid((const u8 *)base, size,
                                    base->modelOffsets[i],
                                    ModelPrimitiveStride, -1)) {
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
                                       size) ||
            !PrimitiveStreamIsValid((const u8 *)base, size,
                                    entry->modelOffset,
                                    CoursePrimitiveStride,
                                    entry->vertexCount)) {
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
