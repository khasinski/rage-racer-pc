#include "game/prim.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"


/* Scene 34: the still shown after the ending FMV. Fades in, waits 300
 * frames or a confirm press, fades out and returns to scene 2. */
void UpdateEndingStill(void) {
    g_SceneTimer++;
    if (g_SceneTimer == 2) {
        SetDispMask(1);
    }

    g_FadeLevel += g_FadeStep;
    if (g_FadeStep > 0) {
        if (g_FadeLevel > 0x100) {
            g_FadeLevel = 0x100;
            g_FadeStep = 0;
        }
    } else if (g_FadeStep == 0) {
        if (g_SceneTimer == 0x12C || (g_PadPressed & PAD_CONFIRM)) {
            g_FadeLevel = 0x100;
            g_FadeStep = -4;
        }
    } else if (g_FadeLevel == 0) {
        g_SceneId = 2;
    }

    DrawRaceEndBanner(g_FadeLevel);
}

/* The still itself: a 0x100 + 0x40 wide pair of full-height sprites. */
void DrawEndingStill(void) {
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);
    u8 *next = RENDER_PRIM_CURSOR_AS(u8);

    next = GameQueueSprite(ot, next, 0, 0, 0x100, 0xF0, 0, 0, 0x3FDB);
    next = QueueDrawModePrim(ot, next, 6);
    next =
        GameQueueSprite(ot, next, 0x100, 0, 0x40, 0xF0, 0, 0, 0x3FDB);
    RENDER_PRIM_CURSOR_AS(u8) = QueueDrawModePrim(ot, next, 7);
}
