#include "game/asset.h"
#include "game/audio.h"
#include "game/race.h"
#include "game/scene.h"
#include "game/state.h"

void ExitRaceScene(s32 sceneId) {
    g_SceneId = sceneId;
    ForceAllEffectVoicesEnabled(0);
    SetReverbDepth(0, 0);
    if (sceneId == GAME_SCENE_INIT_MENU) {
        RequestSelectBgmAssets();
    }
}
