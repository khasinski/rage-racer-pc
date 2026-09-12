#include "game/angle.h"
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
            stepLimit = (((0x1000 - rawError) / 17) * 2) & ANGLE_MASK;
            direction = CHASE_YAW_NEGATIVE;
        } else {
            stepLimit = ((rawError / 17) * 2) & ANGLE_MASK;
            direction = CHASE_YAW_POSITIVE;
        }
    } else if (rawError < -4) {
        if (rawError < -0x7FF) {
            stepLimit = (((0x1000 + rawError) / 17) * 2) & ANGLE_MASK;
            direction = CHASE_YAW_POSITIVE;
        } else {
            stepLimit = ((CameraSubtractWord(0, rawError) / 17) * 2) &
                        ANGLE_MASK;
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
void CameraViewFromChaseCamera(GameCarRuntime *car, GameViewWork *view) {
    Matrix cameraRotation;
    s32 chaseDistance;
    s32 chaseTargetYaw;
    s32 chaseYawLag;
    Vec4 eyeOffset = {0};
    Vec4 eyeWorld = {0};
    Vec4 focusOffset = {0};
    Vec4 focusWorld = {0};
    Matrix inverseObjectRotation;
    Matrix matrixWork;
    s32 pitchOffset;
    Matrix objectRotation;
    s32 settledYaw;

    CameraLoadViewPositionFromCar(view, car);
    chaseTargetYaw = car->bodyYaw & ANGLE_MASK;
    g_ChaseCarSpeed = car->speed;
    g_ChaseTargetYaw = chaseTargetYaw;
    if (g_CameraModePrev == TRACK_CAMERA_CHASE) {
        g_ChaseYawPrev &= ANGLE_MASK;
        g_ChaseYawRampNeg &= ANGLE_MASK;
        g_ChaseYawRampPos &= ANGLE_MASK;
    } else {
        g_ChaseYawPrev = chaseTargetYaw;
        g_ChaseYawRampNeg = 0;
        g_ChaseYawRampPos = 0;
    }
    g_ChaseYawDamping = CalculateChaseYawDamping(g_ChaseCarSpeed);
    UpdateChaseYawStep(g_ChaseTargetYaw, g_ChaseYawPrev);
    settledYaw = CameraAddWord(g_ChaseYawPrev, g_ChaseYawLag) & ANGLE_MASK;
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
    CameraBuildCarRotation(&objectRotation, car);
    TransposeMatrix(&objectRotation, &inverseObjectRotation);
    MulMatrix2(&cameraRotation, &objectRotation);
    TransposeMatrix(&objectRotation, &matrixWork);
    focusOffset.y = -0x3C;
    focusOffset.z = 0x32;
    ApplyMatrixLV(&inverseObjectRotation, AsWords(&focusOffset),
                  AsWords(&focusWorld));
    view->x = CameraAddWord(view->x, focusWorld.x);
    view->y = CameraAddWord(view->y, focusWorld.y);
    view->z = CameraAddWord(view->z, focusWorld.z);
    /* Retail kept both offsets in the same stack slot, so a preset
     * outside 0..2 leaves the eye sitting on the look-at offset. The
     * switch has no default and the eye starts on that offset so it
     * still behaves that way. */
    eyeOffset.y = focusOffset.y;
    eyeOffset.z = focusOffset.z;
    switch (g_ChaseCameraPreset) {
    case 0:
        eyeOffset.y = 0x3A;
        eyeOffset.z = 0x118;
        break;
    case 1:
        eyeOffset.y = 0x59;
        eyeOffset.z = 0x140;
        break;
    case 2:
        eyeOffset.y = 0x97;
        eyeOffset.z = 0x190;
        break;
    }
    ApplyMatrixLV(&matrixWork, AsWords(&eyeOffset), AsWords(&eyeWorld));
    view->x = CameraSubtractWord(view->x, eyeWorld.x);
    view->y = CameraSubtractWord(view->y, eyeWorld.y);
    view->z = CameraSubtractWord(view->z, eyeWorld.z);
    chaseDistance = SquareRoot0(CameraAddWord(
        CameraMultiplyWord(eyeWorld.x, eyeWorld.x),
        CameraMultiplyWord(eyeWorld.z, eyeWorld.z)));
    view->angleX = 0x400 -
        (Atan2(CameraAddWord(eyeWorld.y, 0x28), chaseDistance) & ANGLE_MASK);
    view->angleY = 0x400 - (Atan2(eyeWorld.x, eyeWorld.z) & ANGLE_MASK);
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
