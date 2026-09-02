#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"
#include "game/track_internal.h"

/* The looping prop's live orientation: three 12-bit angles copied wholesale out
 * of the current rotation keyframe by InitPathScenery, which sees the same
 * eight bytes as one Blk8. */
void DrawPathScenery(void) {
    Matrix mtx0;
    Matrix mtx1;
    s32 spinAngle;

    BuildRotMatrixY(&mtx0, 0x800 - g_PathSceneryTransform.rotation.vy);
    BuildRotMatrixX(&mtx1, g_PathSceneryTransform.rotation.vx);
    MulMatrix2(&mtx0, &mtx1);
    MulMatrix2(&g_RenderState.matrix, &mtx1);
    BuildRotMatrixZ(&mtx0, g_PathSceneryTransform.rotation.vz);
    MulMatrix2(&mtx1, &mtx0);

    SelectModelBank(1);
    SetGteObjectMatrix(&g_ObjectMatrixWork,
                       AsPositionWords(g_PathSceneryTransform.position.w), &mtx0);
    g_RenderState.envMode4 = 0;
    SubmitModel(&g_RenderState, ModelOrFallback(0x23, g_ModelBankCount));

    spinAngle = (s32)((u32)g_SceneTimer * 331u) & 0xFFF;
    BuildRotMatrixY(&mtx1, spinAngle);

    MulMatrix2(&mtx0, &mtx1);
    SetGteObjectMatrix(&g_ObjectMatrixWork,
                       AsPositionWords(g_PathSceneryTransform.position.w), &mtx1);
    g_RenderState.envMode4 = 0;
    SubmitModel(&g_RenderState, ModelOrFallback(0x24, g_ModelBankCount));
}
