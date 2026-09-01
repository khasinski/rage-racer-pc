#include "camera_internal.h"
#include "rage/chase_camera.h"

static void SettleChaseYaw(s32 limit, s32 accel, s32 factor, int negative) {
    if (limit < accel) {
        g_ChaseYawLag = negative ? -limit : limit;
        if (negative) {
            g_ChaseYawRampNeg = SquareRoot0(limit * factor);
        } else {
            g_ChaseYawRampPos = SquareRoot0(limit * factor);
        }
    } else {
        g_ChaseYawLag = negative ? -accel : accel;
    }
}

static void AdvanceChaseYawRamp(s32 stepLimit, int negative) {
    s32 accel;
    s32 ramp;

    if (stepLimit > 0x40) {
        stepLimit = 0x40;
    }
    g_ChaseYawStepLimit = stepLimit;
    ramp = negative ? g_ChaseYawRampNeg : g_ChaseYawRampPos;
    accel = ((ramp + 8) * (ramp + 8)) / g_ChaseYawDamping;
    if (negative) {
        g_ChaseYawRampPos = 0;
        g_ChaseYawRampNeg += 8;
    } else {
        g_ChaseYawRampNeg = 0;
        g_ChaseYawRampPos += 8;
    }
    g_ChaseYawStep = accel;
    SettleChaseYaw(stepLimit, accel, g_ChaseYawDamping, negative);
}

/*
 * Mode 1: the chase camera. It trails the car by one of three preset
 * distances, settling its yaw towards where the car is pointing rather than
 * snapping to it.
 */
void CameraViewFromChaseCamera(GameRenderObject *car, GameViewWork *view) {
    Matrix cameraRotation;
    s32 chaseCarSpeed;
    s32 chaseDistance;
    s32 chaseTargetYaw;
    s32 chaseYawDamping;
    s32 chaseYawLag;
    s32 eyeOffset[3];
    s32 eyeWorld[3];
    s32 focusOffset[3];
    s32 focusWorld[3];
    Matrix inverseObjectRotation;
    Matrix matrixWork;
    s32 negatedAccel;
    Matrix objectRotation;
    s32 previousMode;
    s32 rawAngle;
    s32 speedDamping;
    s32 yawError;
    s32 yawStepAhead;
    s32 yawStepBehind;
    s32 yawStepWrapped;

    CameraLoadViewPositionFromCar(view, car);
    chaseYawDamping = car->angleY;
    chaseTargetYaw = chaseYawDamping & 0xFFF;
    chaseCarSpeed = car->speed;
    g_ChaseCarSpeed = chaseCarSpeed;
    previousMode = g_CameraModePrev;
    g_ChaseTargetYaw = chaseTargetYaw;
    if (previousMode == 1) {
        g_ChaseYawPrev &= 0xFFF;
        g_ChaseYawRampNeg &= 0xFFF;
        g_ChaseYawRampPos &= 0xFFF;
    } else {
        g_ChaseYawPrev = chaseTargetYaw;
        g_ChaseYawRampNeg = 0;
        g_ChaseYawRampPos = 0;
    }
    if (g_ChaseCarSpeed >= 0x321) {
        chaseYawDamping = 0x4E2 - g_ChaseCarSpeed;
        g_ChaseYawDamping = chaseYawDamping;
        if (chaseYawDamping < 6) {
            g_ChaseYawDamping = 6;
        }
        g_ChaseYawDamping = ((((g_ChaseYawDamping * 8) / 50) + 8) / 10) + 1;
    } else {
        speedDamping = 0x4E2 - g_ChaseCarSpeed;
        g_ChaseYawDamping = speedDamping;
        g_ChaseYawDamping =
            ((((g_ChaseYawDamping * 6 * speedDamping) / 2500) -
              ((speedDamping * 0x46) / 50)) + 0xE0) /
            10;
    }
    yawError = g_ChaseTargetYaw - g_ChaseYawPrev;
    if (yawError >= 5) {
        if (yawError >= 0x800) {
            yawStepWrapped = (((0x1000 - yawError) / 17) * 2) & 0xFFF;
            AdvanceChaseYawRamp(yawStepWrapped, 1);
        } else {
            yawStepAhead =
                (((g_ChaseTargetYaw - g_ChaseYawPrev) / 17) * 2) & 0xFFF;
            AdvanceChaseYawRamp(yawStepAhead, 0);
        }
    } else if (yawError < -4) {
        if (yawError < -0x7FF) {
            yawStepWrapped = (((0x1000 - (g_ChaseYawPrev - g_ChaseTargetYaw)) / 17) * 2) & 0xFFF;
            AdvanceChaseYawRamp(yawStepWrapped, 0);
        } else {
            yawStepBehind = (((g_ChaseYawPrev - g_ChaseTargetYaw) / 17) * 2) & 0xFFF;
            AdvanceChaseYawRamp(yawStepBehind, 1);
        }
    } else {
        g_ChaseYawLag = 0;
        g_ChaseYawRampNeg = 0;
        g_ChaseYawRampPos = 0;
    }
    rawAngle = g_ChaseTargetYaw;
    chaseCarSpeed = (g_ChaseYawPrev + g_ChaseYawLag) & 0xFFF;
    g_ChaseYaw = chaseCarSpeed;
    /* How far the chase yaw still has to travel, taken the short way
     * round the circle. Which way that is depends on which side of the
     * target it started. */
    chaseYawLag = rawAngle - chaseCarSpeed;
    if (rawAngle < chaseCarSpeed) {
        if (chaseYawLag < -0x7FF) {
            chaseYawLag += 0x1000;
        }
    } else if (chaseYawLag >= 0x800) {
        chaseYawLag -= 0x1000;
    }
    g_ChaseYawLag = chaseYawLag;
    BuildRotMatrixY(&cameraRotation, 0 - g_ChaseYawLag);
    BuildRotMatrixX(&matrixWork, -0x80);
    MulMatrix2(&matrixWork, &cameraRotation);
    g_ChaseYawPrev = g_ChaseYaw;
    BuildRotMatrixY(&objectRotation, car->angleY);
    BuildRotMatrixX(&matrixWork, car->bodyPitch);
    MulMatrix2(&matrixWork, &objectRotation);
    BuildRotMatrixZ(&matrixWork, car->bodyRoll);
    MulMatrix2(&matrixWork, &objectRotation);
    TransposeMatrix(&objectRotation, &inverseObjectRotation);
    MulMatrix2(&cameraRotation, &objectRotation);
    TransposeMatrix(&objectRotation, &matrixWork);
    focusOffset[0] = 0;
    focusOffset[1] = -0x3C;
    focusOffset[2] = 0x32;
    ApplyMatrixLV(&inverseObjectRotation, &focusOffset[0],
                  &focusWorld[0]);
    view->x += focusWorld[0];
    view->y += focusWorld[1];
    view->z += focusWorld[2];
    /* Retail kept both offsets in the same stack slot, so a preset
     * outside 0..2 leaves the eye sitting on the look-at offset. The
     * switch has no default and the eye starts on that offset so it
     * still behaves that way. */
    eyeOffset[0] = 0;
    eyeOffset[1] = focusOffset[1];
    eyeOffset[2] = focusOffset[2];
    switch (g_ChaseCameraPreset) {
    case 0:
        eyeOffset[1] = 0x3A;
        eyeOffset[2] = 0x118;
        break;
    case 1:
        eyeOffset[1] = 0x59;
        eyeOffset[2] = 0x140;
        break;
    case 2:
        eyeOffset[1] = 0x97;
        eyeOffset[2] = 0x190;
        break;
    }
    ApplyMatrixLV(&matrixWork, &eyeOffset[0], &eyeWorld[0]);
    view->x -= eyeWorld[0];
    view->y -= eyeWorld[1];
    view->z -= eyeWorld[2];
    chaseDistance = SquareRoot0((eyeWorld[0] * eyeWorld[0]) +
                                (eyeWorld[2] * eyeWorld[2]));
    view->angleX = 0x400 - (Atan2(eyeWorld[1] + 0x28, chaseDistance) & 0xFFF);
    view->angleY = 0x400 - (Atan2(eyeWorld[0], eyeWorld[2]) & 0xFFF);
    view->angleY += ChaseCameraYawOffset(car->steeringAngle);
    view->angleZ = car->bodyRoll - car->bodyRollVelocity;
    if (g_ChaseCameraPreset == 0) {
        negatedAccel = view->angleX - 0x90;
    } else {
        negatedAccel = view->angleX - 0x60;
    }
    view->angleX = negatedAccel;
    g_CameraModePrev = 1;
}
