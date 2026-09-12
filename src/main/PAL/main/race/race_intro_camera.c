#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/track.h"

enum { INTRO_CAR_VIEW_HEIGHT = 28 };

void RunRaceIntroCamera(Camera *camera, PlayerCarRuntime *car, s32 mode) {
    GameViewWork viewWork;
    LVec delta;

    if (mode >= 90) {
        UpdateCamera(camera, CAMERA_VIEW_CAR, AsRivalCar(car));
        return;
    }
    if (g_RaceIntroCameraScript == NULL ||
        (mode >= 2 && camera->intro.key == NULL)) {
        UpdateCamera(camera, CAMERA_VIEW_CAR, AsRivalCar(car));
        return;
    }

    LoadViewWork(&viewWork, &camera->view);

    if (mode < 2) {
        const RaceIntroCameraScript *script = g_RaceIntroCameraScript;
        s32 series = g_RaceSeries != 0;
        s16 keyIndex = script->firstKeyIndex[series];
        const RaceIntroCameraKey *key = &script->keys[keyIndex];

        camera->intro.key = key;
        camera->view.x = key->x.word;
        camera->view.y = key->y.word;
        camera->view.z = key->z.word;
        camera->view.parameter = key->mode;
        camera->intro.delta.vx = WrapSigned16(
            (int64_t)key[1].x.half.value - key[0].x.half.value);
        camera->intro.delta.vy = WrapSigned16(
            (int64_t)key[1].y.half.value - key[0].y.half.value);
        camera->intro.delta.vz = WrapSigned16(
            (int64_t)key[1].z.half.value - key[0].z.half.value);
        camera->intro.timer = key[0].duration;
    } else {
        const RaceIntroCameraKey *key = camera->intro.key;

        if (mode == key->startFrame) {
            camera->intro.key = &key[1];
            camera->intro.timer = key[1].duration;
            if (key[1].mode == 1) {
                camera->intro.delta.vx = WrapSigned16(
                    (int64_t)(u16)car->x - key[1].x.half.value);
                camera->intro.delta.vy = WrapSigned16(
                    (int64_t)(u16)car->y - INTRO_CAR_VIEW_HEIGHT -
                    key[1].y.half.value);
                camera->intro.delta.vz = WrapSigned16(
                    (int64_t)(u16)car->z - key[1].z.half.value);
            } else {
                camera->intro.delta.vx = WrapSigned16(
                    (int64_t)key[2].x.half.value - key[1].x.half.value);
                camera->intro.delta.vy = WrapSigned16(
                    (int64_t)key[2].y.half.value - key[1].y.half.value);
                camera->intro.delta.vz = WrapSigned16(
                    (int64_t)key[2].z.half.value - key[1].z.half.value);
            }
        }
    }

    camera->intro.timer--;
    if (camera->intro.timer <= 0) {
        camera->intro.timer = 0;
    }

    if (camera->intro.key->mode == 0) {
        s32 duration = camera->intro.key->duration;
        s32 interpolationAngle = duration > 0
                                     ? (camera->intro.timer << 10) / duration
                                     : 0;
        s32 interpolation = rcos(interpolationAngle);

        viewWork.x = WrapSigned32(
            (int64_t)camera->intro.key->x.word +
            camera->intro.delta.vx * interpolation / 4096);
        viewWork.y = WrapSigned32(
            (int64_t)camera->intro.key->y.word +
            camera->intro.delta.vy * interpolation / 4096);
        viewWork.z = WrapSigned32(
            (int64_t)camera->intro.key->z.word +
            camera->intro.delta.vz * interpolation / 4096);

        delta.x = WrapSigned32(
            (int64_t)rsin(car->bodyYaw) / 128 + car->x - viewWork.x);
        delta.y = WrapSigned32(
            (int64_t)car->y - INTRO_CAR_VIEW_HEIGHT - viewWork.y);
        delta.z = WrapSigned32(
            (int64_t)rcos(car->bodyYaw) / 128 + car->z - viewWork.z);
        viewWork.angleY = ANGLE_QUARTER_TURN - Atan2(delta.x, delta.z);
        viewWork.angleX = ANGLE_QUARTER_TURN -
                          Atan2(delta.y,
                                DistanceXZ(delta.x, delta.z) >> 6);
        viewWork.angleZ = 0;
        StoreViewWork(&camera->view, &viewWork);
        SetCameraRotMatrix(&camera->view);
        SelectModelBank(0);
        DrawPlayerCarModel(AsRivalCar(car));
    } else {
        DrawFullscreenFadeTile(camera->intro.timer * 26, 0x29);
        viewWork.x = car->x;
        viewWork.y = WrapSigned32(
            (int64_t)car->y - INTRO_CAR_VIEW_HEIGHT);
        viewWork.z = car->z;
        viewWork.parameter = car->positionW;
        viewWork.angleX = car->bodyPitch;
        viewWork.angleY = car->bodyYaw;
        viewWork.angleZ = car->bodyRoll;
        viewWork.depth = car->bodyRotationW;
        StoreViewWork(&camera->view, &viewWork);
        SetCameraRotMatrix(&camera->view);
    }
}
