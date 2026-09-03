#include "game/audio.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/track.h"

enum {
    FINISH_CUE_FLAG = 8,
    FIRST_SPEED_CUE_FLAG = 0x10,
    FINISH_SOUND_CUE = 0x2A,
    SPEED_SOUND_CUE = 0x23,
    WRONG_WAY_FINISH_SOUND_LIMIT = 10,
    PERCENT_SCALE = 100,
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
        if (g_WrongWayTimer < WRONG_WAY_FINISH_SOUND_LIMIT) {
            PlaySoundCue(FINISH_SOUND_CUE);
        }
    }
}

static void TriggerSpeedCue(void) {
    const TrackSpeedCue *cues =
        g_TrackEventData->raceCues.speed[g_RaceSeries];
    s32 index;

    for (index = 0; index < TRACK_SPEED_CUE_COUNT; index++) {
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

        speedThreshold = WrapSigned32(
            (int64_t)cue->speedPercent * g_PlayerCar.drive.speedScale);
        speedThreshold /= PERCENT_SCALE;
        if (g_PlayerCar.speed > speedThreshold &&
            g_PlayerCar.motionMode <= 0) {
            g_RaceCueFlags |= flag;
            PlaySoundCue(SPEED_SOUND_CUE);
        }
        return;
    }
}

void TriggerRaceCues(void) {
    if (g_TrackEventData == NULL ||
        (u32)g_RaceSeries >= TRACK_SERIES_COUNT) {
        return;
    }

    TriggerFinishCue();
    if (g_WrongWayTimer == 0) {
        TriggerSpeedCue();
    }
}
