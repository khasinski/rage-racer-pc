#include "game/asset.h"
#include "game/audio.h"
#include "game/frontend_internal.h"
#include "game/menu.h"
#include "game/race.h"

/* Scene 2: reset title/menu state, then hand over to UpdateFrontend. */
void EnterFrontend(void) {
    Frontend *frontend = MenuFrontend();

    SetDispMask(0);
    CloseLoadedAudioSlots();
    ResetTrackTextureSwap();
    UploadLoadBufferImage();

    g_FrameSyncThreshold = 0x80;
    g_SceneId = 4;
    g_SceneTimer = 0;
    *frontend = (Frontend){
        .state = FRONTEND_STATE_TITLE,
        .attractTimer = -1,
    };

    RefreshClassWinState();
    SetDefaultReverbDepth();
}
