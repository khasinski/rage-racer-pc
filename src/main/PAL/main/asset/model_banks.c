#include "game/asset.h"
#include <string.h>
#include "game/asset_internal.h"
#include "game/model_stream.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track.h"

s32 RegisterModelBank(const ModelBankHeader *base, size_t size, s32 index) {
    if ((u32)index >= GAME_MODEL_BANK_LIMIT) return 0;
    return ReadModelBank(base, size, &g_ModelBanks[index]);
}

void SelectModelBank(s32 index) {
    const NativeModelBank *bank;

    if ((u32)index >= GAME_MODEL_BANK_LIMIT) return;
    bank = &g_ModelBanks[index];
    g_RenderState.geometry.modelTable1 = bank->table;
    g_RenderState.geometry.modelNormals = bank->normals;
    g_ModelBankCount = bank->modelCount;
    g_RenderState.geometry.modelModels = bank->models;
}

s32 RegisterCourseModels(const CourseModelAssetHeader *base, size_t size) {
    CourseBank bank;
    if (!ReadCourseBank(base, size, &bank)) return 0;
    memcpy(g_NativeCourseModels, bank.models, sizeof(bank.models));
    g_CourseModelCount = bank.modelCount;
    g_RenderState.geometry.courseBank = g_NativeCourseModels;
    return 1;
}
