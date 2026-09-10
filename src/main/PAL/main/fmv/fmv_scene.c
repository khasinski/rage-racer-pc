#include "game/audio.h"
#include "game/cd.h"
#include "game/fmv.h"
#include "game/fmv_internal.h"
#include "game/scene.h"
#include "game/state.h"

void BeginFmv(s32 returnScene) {
    CloseLoadedAudioSlots();
    ResetCdAudioState();
    g_FmvState = FMV_PLAYBACK_START;
    g_StreamReturnScene = returnScene;
    g_SceneId = GAME_SCENE_FMV;
    StopFmvDiscPlayback();
    /* Race/menu transitions fade the live CD attenuator to zero. XA uses
     * that same mixer, so start the movie with the configured level rather
     * than inheriting a completed (or still active) music fade. */
    g_CdFadeFrames = 0;
    SetCdVolume(g_CdVolume);
}

void UpdateFmv(void) {
    switch (g_FmvState) {
    case FMV_PLAYBACK_INVALID:
        break;
    case FMV_PLAYBACK_START:
        StartFmvPlayback();
        RAGE_FALLTHROUGH;
    case FMV_PLAYBACK_DECODE:
        DecodeFmvFrame();
        break;
    case FMV_PLAYBACK_FINISH:
        EndFmv();
        break;
    default:
        g_FmvState = FMV_PLAYBACK_FINISH;
        EndFmv();
        break;
    }
}
