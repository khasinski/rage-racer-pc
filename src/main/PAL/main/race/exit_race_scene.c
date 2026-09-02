#include <stdio.h>

#include "game/asset.h"
#include "game/audio.h"
#include "game/race.h"
#include "game/state.h"

void ExitRaceScene(s32 sceneId) {
    g_SceneId = sceneId;
    ForceAllEffectVoicesEnabled(0);
    SetReverbDepth(0, 0);
    if (sceneId == 6) {
        RequestSelectBgmAssets();
    }
    printf("%s", g_MsgGameExit);
}
