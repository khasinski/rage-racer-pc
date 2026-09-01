#include "game/audio.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/track.h"

enum {
    FINISH_CUE_FLAG = 8,
    FIRST_SPEED_CUE_FLAG = 0x10,
    SPEED_CUE_COUNT = 3,
};

static void TriggerFinishCue(void) {
    const TrackFinishCue *cue;

    if ((g_RaceCueFlags & FINISH_CUE_FLAG) != 0) {
        return;
    }

    cue = &g_TrackEventData->raceCues.finish[g_RaceSeries];
    if (g_PlayerCar.trackSection == cue->trackSection &&
        g_PlayerCar.lap == g_LapCount) {
        g_RaceCueFlags |= FINISH_CUE_FLAG;
        if (g_WrongWayTimer < 10) {
            PlaySoundCue(0x2A);
        }
    }
}

static void TriggerSpeedCue(void) {
    const TrackSpeedCue *cues =
        g_TrackEventData->raceCues.speed[g_RaceSeries];
    s32 index;

    for (index = 0; index < SPEED_CUE_COUNT; index++) {
        const TrackSpeedCue *cue = &cues[index];
        s32 flag = FIRST_SPEED_CUE_FLAG << index;
        s32 speedThreshold;

        if ((g_RaceCueFlags & flag) != 0) {
            continue;
        }
        if (cue->trackSection == -1) {
            return;
        }
        if (g_PlayerCar.trackSection != cue->trackSection) {
            continue;
        }

        speedThreshold = cue->speedPercent * g_PlayerCar.drive.speedScale / 100;
        if (g_PlayerCar.speed > speedThreshold &&
            g_PlayerCar.motionMode <= 0) {
            g_RaceCueFlags |= flag;
            PlaySoundCue(0x23);
        }
        return;
    }
}

void TriggerRaceCues(void) {
    TriggerFinishCue();
    if (g_WrongWayTimer == 0) {
        TriggerSpeedCue();
    }
}
