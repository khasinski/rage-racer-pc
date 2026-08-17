#include "common.h"
#include <stdio.h>
#include "game/track.h"
#include "game/car.h"
#include "game/race.h"

void InitSoundSystem(void) {
    if (InitSoundWithVab() != 0) {
        printf("%s", &g_MsgSoundError);
    }
    printf("%s", &g_MsgInitSoundOk);
}

void InitEngineSound(void) {
    LoadExtraVabSlotWithTable();
    SetEffectVoicesEnabled(1);
    SetReverbPreset(2, 0, 0);
    printf("%s", &g_MsgInitEngineOk);
}
