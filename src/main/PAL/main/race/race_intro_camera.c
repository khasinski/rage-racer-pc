#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/track.h"

enum { INTRO_CAR_VIEW_HEIGHT = 28 };

void RunRaceIntroCamera(PlayerCarRuntime *car, s32 mode) {
    GameViewWork viewWork;
    s32 delta[3];

    if (mode >= 90) {
        UpdateCamera(CAMERA_VIEW_CAR,
                     GetCarRenderObject(AsRivalCar(car)));
        return;
    }
    if (g_RaceIntroCameraScript == NULL ||
        (mode >= 2 && g_RaceIntroCameraCursor == NULL)) {
        UpdateCamera(CAMERA_VIEW_CAR,
                     GetCarRenderObject(AsRivalCar(car)));
        return;
    }

    LoadViewWork(&viewWork);

    if (mode < 2) {
        const RaceIntroCameraScript *script = g_RaceIntroCameraScript;
        s32 series = g_RaceSeries != 0;
        s16 keyIndex = script->firstKeyIndex[series];
        const RaceIntroCameraKey *key = &script->keys[keyIndex];

        g_RaceIntroCameraCursor = key;
        g_RenderState.viewX = key->x.word;
        g_RenderState.viewY = key->y.word;
        g_RenderState.viewZ = key->z.word;
        g_RenderState.viewParameter = key->mode;
        g_RaceIntroCameraDelta.vx = WrapSigned16(
            (int64_t)key[1].x.half.value - key[0].x.half.value);
        g_RaceIntroCameraDelta.vy = WrapSigned16(
            (int64_t)key[1].y.half.value - key[0].y.half.value);
        g_RaceIntroCameraDelta.vz = WrapSigned16(
            (int64_t)key[1].z.half.value - key[0].z.half.value);
        g_RaceIntroCameraTimer = key[0].duration;
    } else {
        const RaceIntroCameraKey *key = g_RaceIntroCameraCursor;

        if (mode == key->startFrame) {
            g_RaceIntroCameraCursor = &key[1];
            g_RaceIntroCameraTimer = key[1].duration;
            if (key[1].mode == 1) {
                g_RaceIntroCameraDelta.vx = WrapSigned16(
                    (int64_t)(u16)car->x - key[1].x.half.value);
                g_RaceIntroCameraDelta.vy = WrapSigned16(
                    (int64_t)(u16)car->y - INTRO_CAR_VIEW_HEIGHT -
                    key[1].y.half.value);
                g_RaceIntroCameraDelta.vz = WrapSigned16(
                    (int64_t)(u16)car->z - key[1].z.half.value);
            } else {
                g_RaceIntroCameraDelta.vx = WrapSigned16(
                    (int64_t)key[2].x.half.value - key[1].x.half.value);
                g_RaceIntroCameraDelta.vy = WrapSigned16(
                    (int64_t)key[2].y.half.value - key[1].y.half.value);
                g_RaceIntroCameraDelta.vz = WrapSigned16(
                    (int64_t)key[2].z.half.value - key[1].z.half.value);
            }
        }
    }

    g_RaceIntroCameraTimer--;
    if (g_RaceIntroCameraTimer <= 0) {
        g_RaceIntroCameraTimer = 0;
    }

    if (g_RaceIntroCameraCursor->mode == 0) {
        s32 duration = g_RaceIntroCameraCursor->duration;
        s32 interpolationAngle = duration > 0
                                     ? (g_RaceIntroCameraTimer << 10) / duration
                                     : 0;
        s32 interpolation = rcos(interpolationAngle);

        viewWork.x = WrapSigned32(
            (int64_t)g_RaceIntroCameraCursor->x.word +
            g_RaceIntroCameraDelta.vx * interpolation / 4096);
        viewWork.y = WrapSigned32(
            (int64_t)g_RaceIntroCameraCursor->y.word +
            g_RaceIntroCameraDelta.vy * interpolation / 4096);
        viewWork.z = WrapSigned32(
            (int64_t)g_RaceIntroCameraCursor->z.word +
            g_RaceIntroCameraDelta.vz * interpolation / 4096);

        delta[0] = WrapSigned32(
            (int64_t)rsin(car->bodyYaw) / 128 + car->x - viewWork.x);
        delta[1] = WrapSigned32(
            (int64_t)car->y - INTRO_CAR_VIEW_HEIGHT - viewWork.y);
        delta[2] = WrapSigned32(
            (int64_t)rcos(car->bodyYaw) / 128 + car->z - viewWork.z);
        viewWork.angleY = ANGLE_QUARTER_TURN - Atan2(delta[0], delta[2]);
        viewWork.angleX = ANGLE_QUARTER_TURN -
                          Atan2(delta[1],
                                DistanceXZ(delta[0], delta[2]) >> 6);
        viewWork.angleZ = 0;
        StoreViewWork(&viewWork);
        SetCameraRotMatrix();
        SelectModelBank(0);
        DrawPlayerCarModel(GetCarRenderObject(AsRivalCar(car)));
    } else {
        DrawFullscreenFadeTile(g_RaceIntroCameraTimer * 26, 0x29);
        viewWork.x = car->x;
        viewWork.y = WrapSigned32(
            (int64_t)car->y - INTRO_CAR_VIEW_HEIGHT);
        viewWork.z = car->z;
        viewWork.parameter = car->positionW;
        viewWork.angleX = car->bodyPitch;
        viewWork.angleY = car->bodyYaw;
        viewWork.angleZ = car->bodyRoll;
        viewWork.depth = car->bodyRotationW;
        StoreViewWork(&viewWork);
        SetCameraRotMatrix();
    }
}
