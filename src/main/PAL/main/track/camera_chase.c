#include "camera_internal.h"
#include "rage/chase_camera.h"

enum ChaseYawDirection {
    CHASE_YAW_POSITIVE,
    CHASE_YAW_NEGATIVE,
};

/*
 * Settle the chase yaw for one frame. Four paths reach this: the yaw error
 * can be positive or negative, and either can be the short way round or the
 * long way across the wrap. `stepLimit` is how far the camera may swing this
 * frame; `acceleratedStep` is how far its acceleration ramp wants to swing.
 */
static void SettleChaseYaw(s32 stepLimit, s32 acceleratedStep,
                           enum ChaseYawDirection direction) {
    int negative = direction == CHASE_YAW_NEGATIVE;

    if (stepLimit < acceleratedStep) {
        g_ChaseYawLag = negative ? -stepLimit : stepLimit;
        if (negative) {
            g_ChaseYawRampNeg = SquareRoot0(
                CameraMultiplyWord(stepLimit, g_ChaseYawDamping));
        } else {
            g_ChaseYawRampPos = SquareRoot0(
                CameraMultiplyWord(stepLimit, g_ChaseYawDamping));
        }
    } else {
        g_ChaseYawLag = negative ? -acceleratedStep : acceleratedStep;
    }
}

static void AdvanceChaseYawRamp(s32 stepLimit,
                                enum ChaseYawDirection direction) {
    s32 acceleratedStep;
    s32 ramp;
    int negative = direction == CHASE_YAW_NEGATIVE;

    if (stepLimit > 0x40) {
        stepLimit = 0x40;
    }
    g_ChaseYawStepLimit = stepLimit;
    ramp = negative ? g_ChaseYawRampNeg : g_ChaseYawRampPos;
    ramp = CameraAddWord(ramp, 8);
    acceleratedStep = CameraMultiplyWord(ramp, ramp) /
                      g_ChaseYawDamping;
    if (negative) {
        g_ChaseYawRampPos = 0;
        g_ChaseYawRampNeg = CameraAddWord(g_ChaseYawRampNeg, 8);
    } else {
        g_ChaseYawRampNeg = 0;
        g_ChaseYawRampPos = CameraAddWord(g_ChaseYawRampPos, 8);
    }
    g_ChaseYawStep = acceleratedStep;
    SettleChaseYaw(stepLimit, acceleratedStep, direction);
}

static s32 CalculateChaseYawDamping(s32 carSpeed) {
    s32 speedDifference = CameraSubtractWord(0x4E2, carSpeed);
    s32 damping;

    if (carSpeed >= 0x321) {
        if (speedDifference < 6) {
            speedDifference = 6;
        }
        return ((((speedDifference * 8) / 50) + 8) / 10) + 1;
    }
    damping = CameraMultiplyWord(speedDifference, 6);
    damping = CameraMultiplyWord(damping, speedDifference) / 2500;
    damping = CameraSubtractWord(
        damping, CameraMultiplyWord(speedDifference, 0x46) / 50);
    damping = CameraAddWord(damping, 0xE0) / 10;
    return damping > 0 ? damping : 1;
}

static void UpdateChaseYawStep(s32 targetYaw, s32 previousYaw) {
    s32 rawError = CameraSubtractWord(targetYaw, previousYaw);
    s32 stepLimit;
    enum ChaseYawDirection direction;

    if (rawError >= 5) {
        if (rawError >= 0x800) {
            stepLimit = (((0x1000 - rawError) / 17) * 2) & 0xFFF;
            direction = CHASE_YAW_NEGATIVE;
        } else {
            stepLimit = ((rawError / 17) * 2) & 0xFFF;
            direction = CHASE_YAW_POSITIVE;
        }
    } else if (rawError < -4) {
        if (rawError < -0x7FF) {
            stepLimit = (((0x1000 + rawError) / 17) * 2) & 0xFFF;
            direction = CHASE_YAW_POSITIVE;
        } else {
            stepLimit = ((CameraSubtractWord(0, rawError) / 17) * 2) &
                        0xFFF;
            direction = CHASE_YAW_NEGATIVE;
        }
    } else {
        g_ChaseYawLag = 0;
        g_ChaseYawRampNeg = 0;
        g_ChaseYawRampPos = 0;
        return;
    }
    AdvanceChaseYawRamp(stepLimit, direction);
}

/*
 * Mode 1: the chase camera. It trails the car by one of three preset
 * distances, settling its yaw towards where the car is pointing rather than
 * snapping to it.
 */
void CameraViewFromChaseCamera(GameRenderObject *car, GameViewWork *view) {
    Matrix cameraRotation;
    s32 chaseDistance;
    s32 chaseTargetYaw;
    s32 chaseYawLag;
    s32 eyeOffset[3];
    s32 eyeWorld[3];
    s32 focusOffset[3];
    s32 focusWorld[3];
    Matrix inverseObjectRotation;
    Matrix matrixWork;
    s32 pitchOffset;
    Matrix objectRotation;
    s32 settledYaw;

    CameraLoadViewPositionFromCar(view, car);
    chaseTargetYaw = car->bodyYaw & 0xFFF;
    g_ChaseCarSpeed = car->speed;
    g_ChaseTargetYaw = chaseTargetYaw;
    if (g_CameraModePrev == TRACK_CAMERA_CHASE) {
        g_ChaseYawPrev &= 0xFFF;
        g_ChaseYawRampNeg &= 0xFFF;
        g_ChaseYawRampPos &= 0xFFF;
    } else {
        g_ChaseYawPrev = chaseTargetYaw;
        g_ChaseYawRampNeg = 0;
        g_ChaseYawRampPos = 0;
    }
    g_ChaseYawDamping = CalculateChaseYawDamping(g_ChaseCarSpeed);
    UpdateChaseYawStep(g_ChaseTargetYaw, g_ChaseYawPrev);
    settledYaw = CameraAddWord(g_ChaseYawPrev, g_ChaseYawLag) & 0xFFF;
    g_ChaseYaw = settledYaw;
    /* How far the chase yaw still has to travel, taken the short way
     * round the circle. Which way that is depends on which side of the
     * target it started. */
    chaseYawLag = CameraSubtractWord(chaseTargetYaw, settledYaw);
    if (chaseTargetYaw < settledYaw) {
        if (chaseYawLag < -0x7FF) {
            chaseYawLag = CameraAddWord(chaseYawLag, 0x1000);
        }
    } else if (chaseYawLag >= 0x800) {
        chaseYawLag = CameraSubtractWord(chaseYawLag, 0x1000);
    }
    g_ChaseYawLag = chaseYawLag;
    BuildRotMatrixY(&cameraRotation,
                    CameraSubtractWord(0, g_ChaseYawLag));
    BuildRotMatrixX(&matrixWork, -0x80);
    MulMatrix2(&matrixWork, &cameraRotation);
    g_ChaseYawPrev = g_ChaseYaw;
    BuildRotMatrixY(&objectRotation, car->bodyYaw);
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
    view->x = CameraAddWord(view->x, focusWorld[0]);
    view->y = CameraAddWord(view->y, focusWorld[1]);
    view->z = CameraAddWord(view->z, focusWorld[2]);
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
    view->x = CameraSubtractWord(view->x, eyeWorld[0]);
    view->y = CameraSubtractWord(view->y, eyeWorld[1]);
    view->z = CameraSubtractWord(view->z, eyeWorld[2]);
    chaseDistance = SquareRoot0(CameraAddWord(
        CameraMultiplyWord(eyeWorld[0], eyeWorld[0]),
        CameraMultiplyWord(eyeWorld[2], eyeWorld[2])));
    view->angleX = 0x400 -
        (Atan2(CameraAddWord(eyeWorld[1], 0x28), chaseDistance) & 0xFFF);
    view->angleY = 0x400 - (Atan2(eyeWorld[0], eyeWorld[2]) & 0xFFF);
    view->angleY += ChaseCameraYawOffset(car->steeringAngle);
    view->angleZ = CameraSubtractWord(car->bodyRoll,
                                      car->bodyRollVelocity);
    if (g_ChaseCameraPreset == 0) {
        pitchOffset = view->angleX - 0x90;
    } else {
        pitchOffset = view->angleX - 0x60;
    }
    view->angleX = pitchOffset;
    g_CameraModePrev = TRACK_CAMERA_CHASE;
}
