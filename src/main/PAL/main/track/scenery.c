#include "common.h"
#include "game/scene_manager.h"
#include "game/audio.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/random.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/scratchpad.h"
#include "game/state.h"
#include "game/track.h"
#include "game/track_internal.h"
#include "game/player_car_internal.h"
#include "game/vector.h"
#include "psyq/gte.h"


static inline void ClearScratchRenderMode3DF68(void) {
    SCRATCH_ENV_MODE4 = 0;
}

void DrawStaticScenery(s32 shifted) {
    Matrix mtx;
    Vec4 state;
    s32 wordIndex;
    s32 bitIndex;
    s32 value;
    u32 *visibility;
    u32 *wordPtr;
    VisibilityMaskAddress visibilityAddress;
    s32 bit;
    s32 visible;
    s16 drawArg;
    s32 frameValue;

    state = g_StaticSceneryPos;

    if (shifted != 0) {
        state.z += 0x5000;
    }

    wordIndex = state.z + 0x400;
    if (wordIndex < 0) {
        wordIndex = state.z + 0xBFF;
    }
    wordIndex >>= 11;

    value = state.x;
    visibility = g_VisibleCellMask;
    bit = value + 0x400;
    visibilityAddress.pointer = visibility;
    visibilityAddress.value = (wordIndex << 2) + visibilityAddress.value;
    wordPtr = visibilityAddress.pointer;
    if (bit < 0) {
        bit = value + 0xBFF;
    }
    bitIndex = bit >> 11;
    visible = 1 << bitIndex;
    visible &= *wordPtr;

    if (visible != 0) {
        BuildRotMatrixY(&mtx, g_StaticSceneryYaw);
        MulMatrix2(SCRATCH_VIEW_MATRIX_GTE, &mtx);

        if (g_IsEnvironmentMode4 != 0) {
            SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, &state, &mtx);
            frameValue = g_CourseModelCount;
            ClearScratchRenderMode3DF68();
            drawArg = 1;
            if (frameValue >= 0x3B) {
                drawArg = 0x3A;
            }
            SubmitCourseModel(SCRATCHPAD, drawArg);
        } else {
            SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, &state, &mtx);
            frameValue = g_CourseModelCount;
            ClearScratchRenderMode3DF68();
            drawArg = 1;
            if (frameValue >= 0x3A) {
                drawArg = 0x39;
            }
            SubmitCourseModel2(SCRATCHPAD, drawArg);
        }
    }
}

void DrawHighClassScenery(void) {
    Matrix mtx;
    s32 pad[4];
    s32 *state;
    s32 drawArg;

    (void)pad;
    state = &g_HighClassSceneryYaw;
    BuildRotMatrixY(&mtx, state[0]);
    MulMatrix2(SCRATCH_VIEW_MATRIX_GTE, &mtx);

    if (g_IsEnvironmentMode4 != 0) {
        SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, state - 3, &mtx);
        SCRATCH_ENV_MODE4 = 0x10000;
        drawArg = 1;
        if (g_CourseModelCount >= 0x40) {
            drawArg = 0x3F;
        }
        SubmitCourseModel(SCRATCHPAD, drawArg);
    } else {
        SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, state - 3, &mtx);
        SCRATCH_ENV_MODE4 = 0;
        drawArg = 1;
        if (g_CourseModelCount >= 0x40) {
            drawArg = 0x3F;
        }
        SubmitCourseModel2(SCRATCHPAD, drawArg);
    }
}

void DrawCourseScenery(s32 course, s32 timer, s32 animate) {
    s32 mode = course;
    s32 value = timer;
    s32 flag = animate;

    DrawAnimatedScenery(value, 0);

    if (g_GrandPrixClass == 5) {
        flag = 0;
    }

    switch (mode) {
    case 0:
        DrawSpinningScenery(value, flag);
        if (g_GrandPrixClass >= 4) {
            DrawHighClassScenery();
        }
        DrawStaticScenery(0);
        break;
    case 1:
        if (g_GrandPrixClass >= 2) {
            DrawSpinningScenery(value, flag);
        }
        if (flag != 0) {
            UpdateShuttleScenery(0);
        }
        DrawShuttleScenery(0);
        DrawStaticScenery(0);
        break;
    case 2:
        if (flag != 0) {
            UpdateShuttleScenery(0);
            UpdateShuttleScenery(1);
        }
        DrawShuttleScenery(0);
        DrawShuttleScenery(1);
        DrawStaticScenery(0);
        break;
    case 3:
        DrawAnimatedScenery(value, 1);
        DrawStaticScenery(1);
        break;
    default:
        break;
    }
}

void DrawCourseScenery2(s32 timer, s32 animate) {
    s32 value = timer;
    s32 flag = animate;
    s32 mode;

    if (g_GrandPrixClass == 5) {
        flag = 0;
    }

    DrawAnimatedScenery2(value, 0, g_SceneId == SCENE_REPLAY, flag);

    mode = RageSeriesCourseIndex();
    switch (mode) {
    case 0:
        DrawSpinningScenery(value, flag);
        if (g_GrandPrixClass >= 4) {
            DrawHighClassScenery();
        }
        DrawStaticScenery(0);
        break;
    case 1:
        if (g_GrandPrixClass >= 2) {
            DrawSpinningScenery(value, flag);
        }
        if (flag != 0) {
            UpdateShuttleScenery(0);
        }
        DrawShuttleScenery(0);
        DrawStaticScenery(0);
        break;
    case 2:
        if (flag != 0) {
            UpdateShuttleScenery(0);
            UpdateShuttleScenery(1);
        }
        DrawShuttleScenery(0);
        DrawShuttleScenery(1);
        DrawStaticScenery(0);
        break;
    case 3:
        DrawAnimatedScenery2(value, 1, g_SceneId == SCENE_REPLAY, flag);
        DrawStaticScenery(1);
        break;
    default:
        break;
    }
}


void SeedFlybyScenery(void) {
    SceneryMotionData *data;
    FlybySceneryState *out;
    s32 count;
    s16 value;
    s32 index;
    s32 cmp;
    s32 scene0;
    s32 scene1;
    s32 recordIndex;

    data = g_FlybySceneryData;
    out = &g_FlybyScenery;
    index = Random15();
    count = g_LapCount;
    value = index % count;
    value++;
    out->lap = value;
    value = (s16)value;

    if (value <= 0) {
        out->lap = (u16)g_LapCount - 1;
    } else {
        cmp = count < value;
        if (cmp != 0) {
            out->lap = (u16)g_LapCount;
        }
    }

    scene0 = g_RaceSeries;
    out->soundEnabled = 1;
    scene1 = g_RaceSeries;
    out->timer = 0;

    out->position = (scene0 + data->start)->position;
    recordIndex = (scene1 + data->firstKeyframe)[0][0];
    out->volume = 0;
    g_FlybySceneryKeyframe = &data->keyframes[recordIndex];
}

/*
 * Ticks the course's one scripted airborne prop and its engine sound; the model
 * is submitted elsewhere. Armed by a scene-counter match, then runs 451 frames,
 * integrating position from a keyframed heading and feeding a distance-attenuated
 * volume to SetPitchedSoundCue.
 */
void UpdateFlybyScenery(void) {
    Matrix mtxY;
    Matrix mtxX;
    s16 dir[4];
    s32 step[4];
    SceneryMotionData *data;
    FlybySceneryState *state;
    s32 series;
    s32 recordIndex;
    SceneryMotionKeyframe *kf;
    Matrix *mx;
    s32 dt;
    s32 cue;
    s32 pitch;
    s32 vol;
    s32 dist;
    s32 dx;
    s32 dy;
    s32 dz;

    data = g_FlybySceneryData;
    state = &g_FlybyScenery;

    if (g_PlayerCar.lap == state->lap) {
        series = g_RaceSeries;
        if (g_PlayerCar.trackSection == (series + data->triggerSection)[0][0]) {
            state->soundEnabled = 1;
            state->timer = 1;
            state->keyframeTime = 0;
            state->lap = 0;
            state->keyframeIndex = 0;
            state->position = (series + data->start)->position;
            state->rotationZ = 0;
            state->rotationY = 0;
            state->rotationX = 0;
            recordIndex = (g_RaceSeries + data->firstKeyframe)[0][0];
            g_FlybySceneryKeyframe = &data->keyframes[recordIndex];
        }
    }

    if (state->timer > 0) {
        state->timer = state->timer + 1;
        state->keyframeTime = state->keyframeTime + 1;
        if (state->timer >= 0x1C3) {
            state->timer = 0;
        }
        if (g_FlybySceneryKeyframe[state->keyframeIndex].duration ==
            state->keyframeTime) {
            state->keyframeIndex = state->keyframeIndex + 1;
            state->keyframeTime = 0;
        }
        if (g_FlybySceneryKeyframe[state->keyframeIndex].duration == -1) {
            state->keyframeIndex = 0;
        }
        kf = g_FlybySceneryKeyframe;
        state->rotationX =
            (kf[state->keyframeIndex + 1].rotationX * state->keyframeTime +
             kf[state->keyframeIndex].rotationX *
                 (dt = kf[state->keyframeIndex].duration - state->keyframeTime)) /
            kf[state->keyframeIndex].duration;
        state->rotationY =
            (kf[state->keyframeIndex + 1].rotationY * state->keyframeTime +
             kf[state->keyframeIndex].rotationY * dt) /
            kf[state->keyframeIndex].duration;
        state->rotationZ =
            (kf[state->keyframeIndex + 1].rotationZ * state->keyframeTime +
             kf[state->keyframeIndex].rotationZ * dt) /
            kf[state->keyframeIndex].duration;
        dir[0] = 0;
        dir[1] = 0;
        dir[2] = -kf[state->keyframeIndex].speed * 4;
        BuildRotMatrixY(&mtxY, 0x800 - state->rotationY);
        mx = &mtxX;
        BuildRotMatrixX(mx, state->rotationX);
        MulMatrix2(&mtxY, mx);
        BuildRotMatrixZ(&mtxY, state->rotationZ);
        MulMatrix(mx, &mtxY);
        ApplyMatrix(mx, dir, step);
        state->position.x = step[0] / 4 + state->position.x;
        state->position.y = step[1] / 4 + state->position.y;
        state->position.z = step[2] / 4 + state->position.z;
        if (state->soundEnabled == 1) {
            dx = g_PlayerCar.x - state->position.x;
            dy = g_PlayerCar.y - state->position.y;
            dz = g_PlayerCar.z - state->position.z;
            dist = SquareRoot12(dx * dx / 8 + dy * dy / 16 + dz * dz / 8) >> 12;
            if (dist < 0) {
                state->soundEnabled = 0;
                dist = 0x74;
            }
            vol = 0x74 - dist;
            if (vol >= 0x75) {
                vol = 0x74;
            }
            if (vol < 0) {
                vol = 0;
            }
            pitch = 0x1900;
            state->volume = vol;
        } else {
            vol = 0;
            state->volume = 0;
            pitch = 0;
        }
    } else {
        vol = 0;
        state->volume = 0;
        pitch = 0;
    }

    if (g_RacePhase >= 3) {
        pitch = 0;
        vol = 0;
    }

    switch (RageSeriesCourseIndex()) {
    default:
        cue = 1;
        pitch = 0;
        vol = 0;
        break;
    case 0:
        cue = 1;
        break;
    case 1:
        cue = 1;
        pitch = 0;
        vol = 0;
        break;
    case 3:
        cue = 2;
        break;
    case 2:
        cue = 2;
        break;
    }
    SetPitchedSoundCue(cue, pitch, vol);
}
