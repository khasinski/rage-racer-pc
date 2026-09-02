#include "game/audio.h"
#include "game/car.h"
#include "game/cd.h"
#include "game/player_car_internal.h"
#include "game/prim.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/replay_internal.h"
#include "game/state.h"
#include "game/track.h"
#include "game/track_internal.h"

void BeginReplay(void) {
    g_FadeLevel = 0xFF;
    g_SceneTimer = 0;
    g_FadeStep = -4;

    if (g_ReplayBufferWrapped != 0) {
        g_ReplayReadCursor = (g_ReplayWriteCursor & -2) + 2;
    } else {
        g_ReplayReadCursor = 0;
        g_ReplayFrameCount = g_ReplayWriteCursor - 2;
    }

    if (g_ReplayReadCursor >= g_ReplayFrameCount) {
        g_ReplayReadCursor = 0;
    }

    if (g_GrandPrixClass != 5) {
        const s32 rewindFrames = g_GrandPrixMode != 0 ? 1800 : 3000;

        SeekEnvironmentScript(g_EnvScriptClock - rewindFrames);
    }

    SeedReplayCars();
}

static void DrawReplayBadge(void) {
    u8 *base;
    u8 *next;

    if ((g_SceneTimer & 0x10) == 0 || g_SeriesCleared != 0) {
        return;
    }

    base = (u8 *)GamePrimaryOrderingTable(0);
    next = GameQueueSprite(base, RENDER_PRIM_CURSOR_AS(u8), 0x10, 0x10, 0x48,
                           0x10, 0, 0x68, 0x780D);
    RENDER_PRIM_CURSOR_AS(u8) = QueueDrawModePrim(base, next, 9);
}

static s32 ClampWashLevel(s32 value) {
    if (value < 0) {
        return 0;
    }
    return value > 0xFF ? 0xFF : value;
}

static void UpdateReplayFade(void) {
    if (g_FadeStep < 0) {
        g_FadeLevel += g_FadeStep;
        if (g_FadeLevel < 0) {
            g_FadeLevel = 0;
            g_FadeStep = 0;
            g_EndingWashLevel = 0;
        }
        DrawFullscreenFadeTile(g_FadeLevel, 0x29);
        return;
    }

    if (g_SeriesCleared != 0 &&
        (u32)(g_ReplayFrameCount - 600) < (u32)g_SceneTimer) {
        g_EndingWashLevel = ClampWashLevel(
            g_SceneTimer + 600 - (s32)g_ReplayFrameCount);
    }

    if (g_FadeStep == 0) {
        if (g_PadPressed & PAD_CONFIRM) {
            g_FadeStep = 4;
            StartCdVolumeFade(0x3C);
        } else if (g_SceneTimer == g_ReplayFrameCount - 68) {
            g_FadeStep = 4;
            if (g_ReplayBufferWrapped == 0) {
                StartCdVolumeFade(0x3C);
            }
        }
    } else {
        g_FadeLevel += g_FadeStep;
        if (g_FadeLevel >= 257) {
            g_MirrorMode = 0;
            g_SceneId = g_GrandPrixMode == 0 ? 0x14 : 0x12;
        }
    }

    if (g_SeriesCleared != 0) {
        if ((u32)(g_ReplayFrameCount - 600) < (u32)g_SceneTimer ||
            g_FadeLevel != 0) {
            DrawSeriesClearedWash(g_EndingWashLevel, g_FadeLevel);
        }
    } else if (g_FadeLevel != 0) {
        DrawFullscreenFadeTile(g_FadeLevel, 0x49);
    }
}

void UpdateReplayScene(void) {
    g_AnimTimer++;
    g_SceneTimer++;
    if (g_SceneTimer == 0x3C && g_GrandPrixMode != 0 &&
        g_SeriesCleared == 0) {
        PlaySoundCue(g_RacePosition == 1 ? 0x40 : 0x41);
    }

    UpdateReplayFade();

    ApplyReplayFrame(g_ReplayReadCursor, AsRivalCar(&g_PlayerCar),
                     &g_Cars[0]);
    g_ReplayReadCursor++;
    if (g_ReplayReadCursor == g_ReplayFrameCount) {
        g_ReplayReadCursor = 0;
    }
    UpdateReplayCars();
    UpdateCamera(CAMERA_VIEW_TRACK, (GameRenderObject *)&g_PlayerCar);
    g_RenderState.envMode4 = g_IsEnvironmentMode4;
    DrawTerrainCellsWide();
    if (g_GrandPrixMode != 0) {
        DrawPlayerCarOnly();
    }
    DrawCourseObjects();
    DrawCourseScenery2(g_SceneTimer, 1);
    UpdateEnvironment();
    DrawSkyBackground();
    DrawReplayBadge();
    if (g_SceneTimer == 1) {
        SetTrackTexturePageNow(g_PlayerCar.trackSection);
    }
}
