#include "common.h"
#include "game/prim.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/scratchpad.h"
#include "game/state.h"
#include "psyq/gpu.h"


/* Scene 34: the still shown after the ending FMV. Fades in, waits 300
 * frames or a confirm press, fades out and returns to scene 2. */
void UpdateEndingStill(void) {
    s32 v0, v1;
    if ((g_SceneTimer = g_SceneTimer + 1) == 2) {
        SetDispMask(1);
    }
    v0 = g_FadeLevel + g_FadeStep;
    g_FadeLevel = v0;
    v1 = g_FadeStep;
    if (v1 > 0) {
        if (v0 >= 257) {
            g_FadeLevel = 0x100;
            g_FadeStep = 0;
        }
    } else if (v1 == 0) {
        if (g_SceneTimer == 0x12C || (g_PadPressed & PAD_CONFIRM)) {
            g_FadeLevel = 0x100;
            g_FadeStep = -4;
        }
    } else if (v0 == 0) {
        g_SceneId = 2;
    }
    DrawRaceEndBanner(g_FadeLevel);
}

/* The still itself: a 0x100 + 0x40 wide pair of full-height sprites. */
void DrawEndingStill(void) {
    u8 *base;
    s32 clut;
    s32 height;
    u8 *volatile *scratch;
    u8 *next;

    base = (u8 *)GamePrimaryOrderingTable(0);
    height = 0xF0;
    clut = 0x3FDB;
    scratch = SCRATCH_PRIM_CURSOR_SLOT;

    next = *scratch;
    next = GameQueueSprite(base, next, 0, 0, 0x100, height, 0, 0, clut);
    next = QueueDrawModePrim(base, next, 6);
    next = GameQueueSprite(base, next, 0x100, 0, 0x40, height, 0, 0, clut);
    *scratch = QueueDrawModePrim(base, next, 7);
}
