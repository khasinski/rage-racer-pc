#include "game/menu.h"
#include "game/render.h"
#include "game/render_state.h"

void DrawTimeAttackPlate(s32 step) {
    s32 progress;

    if (step == 0) {
        g_TimeAttackPlateProgress = 0;
        return;
    }

    if (step < 0) {
        g_TimeAttackPlateProgress += step;
        if (g_TimeAttackPlateProgress < 0) {
            g_TimeAttackPlateProgress = 0;
        }
    }

    progress = g_TimeAttackPlateProgress;
    if (progress != 0) {
        s16 top = (s16)(0xD7 - progress);
        s16 bottom = (s16)(0xD8 + progress);

        GameDrawTexturedQuad(
            RENDER_OT_BASE, 0x4C, top, 0x7C, top, 0x4C, bottom, 0x7C,
            bottom, 0xCC, 0x38, 0xFC, 0x38, 0xCC, 0x50, 0xFC, 0x50,
            0, 0, 0, 0x20F, 1, 0, 0x1C);
    }

    if (step > 0) {
        g_TimeAttackPlateProgress += step;
        if (g_TimeAttackPlateProgress >= 0xD) {
            g_TimeAttackPlateProgress = 0xC;
        }
    }
}
