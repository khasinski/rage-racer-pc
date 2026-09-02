#include "psyq/cd.h"
#include "game/fmv.h"
#include "game/state.h"
#include "game/fmv_internal.h"
#include "game/cd.h"
#include "game/audio.h"

enum {
    FMV_SCENE_ID = 5,
};

void BeginFmv(s32 returnScene) {
    CloseLoadedAudioSlots();
    ResetCdAudioState();
    g_FmvState = FMV_PLAYBACK_START;
    g_StreamReturnScene = returnScene;
    g_SceneId = FMV_SCENE_ID;
    CdSync(0, NULL);
    CdControl(CD_DRIVE_PAUSE, NULL, NULL);
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
    }
}
