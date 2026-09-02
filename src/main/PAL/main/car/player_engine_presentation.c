#include "game/audio.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/random.h"
#include "game/render.h"
#include "game/state.h"

#include "rage/trace.h"

static void SettleDisplayedEngineRpm(const GameCarDrive *drive) {
    s32 shown = g_EngineRpm;
    s32 gap = drive->engineRpm - shown;
    s32 limit = g_CarSpec->revLimit;

    shown += drive->clutch > 0 ? gap / 2 : gap / 4;
    if (shown >= limit) {
        shown = limit;
    } else if (shown < 500) {
        shown = 500;
    }
    g_EngineRpm = shown;
}

static void UpdatePlayerEngineAudio(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 revFlag = 0;

    if (g_EngineRpm >= g_CarSpec->revLimit - 100 &&
        drive->acceleratorInput.value >= 129) {
        g_TachoNeedleFlash = g_AnimTimer & 2;
        g_EngineRpmJitter = Random15() % 150 / 2;
    } else if (drive->engineRpm == 0 && (g_AnimTimer & 8)) {
        g_TachoNeedleFlash = 0;
        g_EngineRpmJitter = rsin(Random15() & 0xFFF) * 150 / 4096;
        if (g_EngineRpmJitter <= 0) {
            g_EngineRpmJitter = 0;
        }
        revFlag = g_EngineRpmJitter < 37;
    } else {
        g_EngineRpmJitter = 0;
        g_TachoNeedleFlash = 0;
    }

    g_EngineRpmSnapshot = g_EngineRpm;
    if (drive->engineRpm != 0) {
        if (drive->gear == 1) {
            revFlag = 1;
        } else if (g_EngineRpm >= g_CarSpec->redline - 2000) {
            revFlag = g_EngineRpm >= g_CarSpec->redline
                ? 1
                : Random15() & 1;
        } else {
            revFlag = 0;
        }
    }

    if (g_RacePhase >= 4) {
        SetIndexedEffectVoice(-1, 0, 0);
    }

    if (drive->manual != 0) {
        UpdateLoadedAudioVoices(
            g_EngineRpm + g_EngineRpmJitter,
            (drive->acceleratorInput.value > 0) && drive->clutch == 0 &&
                revFlag);
    } else {
        UpdateLoadedAudioVoices(
            g_EngineRpm + g_EngineRpmJitter,
            drive->acceleratorInput.value > 0 ? revFlag & 1 : 0);
    }

    drive->gearDisp = drive->gear;
    TraceCarMotion("post-update", car);
}

void UpdatePlayerEnginePresentation(PlayerCarRuntime *car) {
    SettleDisplayedEngineRpm(&car->drive);
    UpdatePlayerEngineAudio(car);
}
