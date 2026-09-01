#include "psyq/cd.h"
#include "game/fmv.h"
#include "game/asset.h"
#include "game/state.h"
#include "game/fmv_internal.h"
#include "game/cd.h"
#include "game/audio.h"

void BeginFmv(s32 returnScene) {
    CloseLoadedAudioSlots();
    ResetCdAudioState();
    g_FmvState = FMV_PLAYBACK_START;
    g_StreamReturnScene = returnScene;
    g_SceneId = 5;
    CdSync(0, 0);
    CdControl(9, 0, 0);
}


void UpdateFmv(void) {
    switch (g_FmvState) {
    case FMV_PLAYBACK_INVALID:
        break;
    case FMV_PLAYBACK_START:
        StartFmvPlayback((FmvWorkBuffers *)(void *)g_AssetBase);
        /* fall through */
    case FMV_PLAYBACK_DECODE:
        DecodeFmvFrame();
        break;
    case FMV_PLAYBACK_FINISH:
        EndFmv();
        break;
    }
}
