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
static void SettleChaseYaw(CameraChase *chase, s32 stepLimit,
                           s32 acceleratedStep,
                           enum ChaseYawDirection direction) {
    int negative = direction == CHASE_YAW_NEGATIVE;

    if (stepLimit < acceleratedStep) {
        chase->yawLag = negative ? -stepLimit : stepLimit;
        if (negative) {
            chase->rampNeg = SquareRoot0(
                CameraMultiplyWord(stepLimit, chase->damping));
        } else {
            chase->rampPos = SquareRoot0(
                CameraMultiplyWord(stepLimit, chase->damping));
        }
    } else {
        chase->yawLag = negative ? -acceleratedStep : acceleratedStep;
    }
}

static void AdvanceChaseYawRamp(CameraChase *chase, s32 stepLimit,
                                enum ChaseYawDirection direction) {
    s32 acceleratedStep;
    s32 ramp;
    int negative = direction == CHASE_YAW_NEGATIVE;

    if (stepLimit > 0x40) {
        stepLimit = 0x40;
    }
    chase->stepLimit = stepLimit;
    ramp = negative ? chase->rampNeg : chase->rampPos;
    ramp = CameraAddWord(ramp, 8);
    acceleratedStep = CameraMultiplyWord(ramp, ramp) /
                      chase->damping;
    if (negative) {
        chase->rampPos = 0;
        chase->rampNeg = CameraAddWord(chase->rampNeg, 8);
    } else {
        chase->rampNeg = 0;
        chase->rampPos = CameraAddWord(chase->rampPos, 8);
    }
    chase->step = acceleratedStep;
    SettleChaseYaw(chase, stepLimit, acceleratedStep, direction);
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

static void UpdateChaseYawStep(CameraChase *chase, s32 targetYaw,
                               s32 previousYaw) {
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
        chase->yawLag = 0;
        chase->rampNeg = 0;
        chase->rampPos = 0;
        return;
    }
    AdvanceChaseYawRamp(chase, stepLimit, direction);
}

/*
 * Mode 1: the chase camera. It trails the car by one of three preset
 * distances, settling its yaw towards where the car is pointing rather than
 * snapping to it.
 */
void CameraViewFromChaseCamera(Camera *camera, GameCarRuntime *car, GameViewWork *view) {
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
    camera->chase.carSpeed = car->speed;
    camera->chase.targetYaw = chaseTargetYaw;
    if (camera->previousMode == TRACK_CAMERA_CHASE) {
        camera->chase.previousYaw &= ANGLE_MASK;
        camera->chase.rampNeg &= ANGLE_MASK;
        camera->chase.rampPos &= ANGLE_MASK;
    } else {
        camera->chase.previousYaw = chaseTargetYaw;
        camera->chase.rampNeg = 0;
        camera->chase.rampPos = 0;
    }
    camera->chase.damping = CalculateChaseYawDamping(camera->chase.carSpeed);
    UpdateChaseYawStep(&camera->chase, camera->chase.targetYaw,
                       camera->chase.previousYaw);
    settledYaw = CameraAddWord(camera->chase.previousYaw, camera->chase.yawLag) & ANGLE_MASK;
    camera->chase.yaw = settledYaw;
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
    camera->chase.yawLag = chaseYawLag;
    BuildRotMatrixY(&cameraRotation,
                    CameraSubtractWord(0, camera->chase.yawLag));
    BuildRotMatrixX(&matrixWork, -0x80);
    MulMatrix2(&matrixWork, &cameraRotation);
    camera->chase.previousYaw = camera->chase.yaw;
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
    switch (camera->chasePreset) {
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
    if (camera->chasePreset == 0) {
        pitchOffset = view->angleX - 0x90;
    } else {
        pitchOffset = view->angleX - 0x60;
    }
    view->angleX = pitchOffset;
    camera->previousMode = TRACK_CAMERA_CHASE;
}
