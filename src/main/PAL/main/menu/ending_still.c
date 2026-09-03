#include "game/prim.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"

#include <limits.h>
#include <stdint.h>

enum {
    ENDING_STILL_DISPLAY_ENABLE_FRAME = 2,
    ENDING_STILL_WAIT_FRAMES = 300,
    ENDING_STILL_FADE_MAX = 0x100,
    ENDING_STILL_RETURN_SCENE = 2,
};

static void AdvanceEndingStillFade(void) {
    s32 current = g_FadeLevel;
    int64_t fade;

    if (current < 0) current = 0;
    if (current > ENDING_STILL_FADE_MAX) {
        current = ENDING_STILL_FADE_MAX;
    }
    g_FadeLevel = current;
    fade = (int64_t)current + g_FadeStep;

    if (g_FadeStep > 0) {
        if (fade >= ENDING_STILL_FADE_MAX) {
            g_FadeLevel = ENDING_STILL_FADE_MAX;
            g_FadeStep = 0;
        } else {
            g_FadeLevel = fade > 0 ? (s32)fade : 0;
        }
    } else if (g_FadeStep < 0) {
        if (fade <= 0) {
            g_FadeLevel = 0;
            g_SceneId = ENDING_STILL_RETURN_SCENE;
        } else {
            g_FadeLevel = fade < ENDING_STILL_FADE_MAX
                              ? (s32)fade
                              : ENDING_STILL_FADE_MAX;
        }
    }
}

/* Scene 34: the still shown after the ending FMV. Fades in, waits 300
 * frames or a confirm press, fades out and returns to scene 2. */
void UpdateEndingStill(void) {
    s32 fadeStep = g_FadeStep;

    if (g_SceneTimer < INT_MAX) {
        g_SceneTimer++;
    }
    if (g_SceneTimer == ENDING_STILL_DISPLAY_ENABLE_FRAME) {
        SetDispMask(1);
    }

    AdvanceEndingStillFade();
    if (fadeStep == 0) {
        if (g_SceneTimer >= ENDING_STILL_WAIT_FRAMES ||
            (g_PadPressed & PAD_CONFIRM)) {
            g_FadeLevel = ENDING_STILL_FADE_MAX;
            g_FadeStep = -4;
        }
    }

    DrawRaceEndBanner(g_FadeLevel);
}

/* The still itself: a 0x100 + 0x40 wide pair of full-height sprites. */
void DrawEndingStill(void) {
    GameOrderingTableEntry *ot;
    u8 *next;

    if (g_DrawBuffer == NULL || g_RenderState.packetCursor == NULL) return;

    ot = GamePrimaryOrderingTable(0);
    next = RENDER_PRIM_CURSOR_AS(u8);

    next = GameQueueSprite(ot, next, 0, 0, 0x100, 0xF0, 0, 0, 0x3FDB);
    next = QueueDrawModePrim(ot, next, 6);
    next =
        GameQueueSprite(ot, next, 0x100, 0, 0x40, 0xF0, 0, 0, 0x3FDB);
    g_RenderState.packetCursor = QueueDrawModePrim(ot, next, 7);
}
