#include "game/render.h"
#include "game/track_internal.h"
#include "game/player_car_internal.h"
#include "rage/chase_camera.h"

/* Mode-3 camera path: the eye is eased from one track-camera node to the next
 * over `node->duration` frames. Each of offset (a local xyz applied through the
 * car's matrix) and orientation (pitch/yaw/roll/distance) keeps a start value,
 * a delta to the destination and the current interpolated value. Mode-3 nodes
 * therefore store angles in the first four words where modes 2/4 store a world
 * position -- the record is a union keyed on `node->mode`. */
typedef union CameraCarAddress {
    PlayerCarRuntime *player;
    GameRenderObject *renderObject;
    Block16 *blocks;
} CameraCarAddress;

/*
 * Settle the chase yaw for one frame. Four paths reach this: the yaw error
 * can be positive or negative, and either can be the short way round or the
 * long way across the wrap. They differ only in which ramp they charge and
 * which way the lag points, so the decompiler had two of them jump into the
 * bodies of the other two. `limit` is how far the camera may swing this
 * frame, `accel` how far the ramp wants to, and the smaller wins.
 */
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

/*
 * Point the camera at a place in the world: pitch and yaw from the camera to
 * the target, in the game's 0x1000-per-turn angle units, with no roll.
 */
static void AimCameraAt(s32 *scratch, s32 targetX, s32 targetY, s32 targetZ) {
    s32 dx = scratch[2] - targetX;
    s32 dy = scratch[3] - targetY;
    s32 dz = scratch[4] - targetZ;
    scratch[6] = 0x400 - (Atan2(0 - dy, SquareRoot0(dx * dx + dz * dz)) & 0xFFF);
    scratch[7] = 0x400 - (Atan2(0 - dx, 0 - dz) & 0xFFF);
    scratch[8] = 0;
}

void UpdateCamera(CameraViewMode cameraModeSel, GameRenderObject *car) {
    /* The camera sits this far above the car, in the car's own frame. */
    s16 cameraLift[4];
    s32 cameraLiftWorld[4];
    /* Every branch that aims the camera builds two offsets in the car's own
     * frame: the point on the car it looks at, and where the eye sits
     * relative to it. The *World pair is each of those rotated out. */
    s32 focusOffset[3];
    s32 focusWorld[3];
    s32 eyeOffset[3];
    s32 eyeWorld[3];
    s32 focusX;
    s32 focusY;
    s32 focusZ;
    /* The track-camera branches take their offset from the node instead. */
    s32 nodeOffset[3];
    s32 nodeWorld[3];
    s32 blend;
    /* Mode 3 alone reads a roll back out of the finished view. */
    s32 rollProbe[3];
    s32 rollWork[3];
    Matrix objectRotation;
    Matrix matrixWork;
    Matrix cameraRotation;
    Matrix inverseObjectRotation;
    s32 previousMode;
    s32 rawAngle;
    s32 turnLimit;
    s32 turnAccel;
    s32 turnFactor;
    s32 *case3Angle;
    s32 *modeAngle;
    ScratchLegacyViewWords legacyView;
    s32 *scratch;
    s32 cameraMode;
    s32 chaseTargetYaw;
    s32 yawError;
    s32 pathBlend;
    s32 pathYawRelative;
    s32 *pitchDeltaPtr;
    s32 speedDamping;
    s32 pathYaw;
    s32 pathPitch;
    s32 pathOffsetZ;
    s32 pathOffsetY;
    s32 camPathOffset;
    s32 camPathAngle;
    s32 chaseYawDamping;
    s32 chaseYawStepLimit;
    s32 cameraNodeIndex;
    s32 yawStepAhead;
    s32 yawStepWrapped;
    s32 yawStepBehind;
    s32 chaseYawLag;
    s32 chaseDistance;
    s32 pathRoll;
    s32 previousNodeIndex;
    u8 nodeChanged;
    s32 chaseCarSpeed;
    s32 offsetXProduct;
    s32 offsetYProduct;
    s32 offsetZProduct;
    s32 pitchProduct;
    s32 yawProduct;
    s32 rollProduct;
    s32 distProduct;
    s32 pitchDelta;
    s32 negatedAccel;
    GameTrackCameraNode *pathNode;
    GameTrackCameraNode *orbitNode;
    GameTrackCameraNode *chaseNode;
    GameTrackCameraNode *prevNode;
    CameraCarAddress playerAddress;
    ScratchBlockAddress scratchAddress;

    cameraNodeIndex = FindNearestTrackCamera(car);
    LoadScratchLegacyView(&legacyView);
    scratch = legacyView.words;
    previousNodeIndex = g_CameraNodeIndex;
    g_CameraNodeIndex = cameraNodeIndex;
    nodeChanged = cameraNodeIndex != previousNodeIndex;
    if (cameraModeSel < 2) {
        cameraMode = cameraModeSel;
    } else {
        cameraMode = g_TrackCameras[cameraNodeIndex].mode;
    }
    switch (cameraMode) {
    case 0:
        playerAddress.renderObject = car;
        scratchAddress.words = &scratch[2];
        scratchAddress.blocks[0] = playerAddress.blocks[0];
        scratchAddress.words = &scratch[6];
        scratchAddress.blocks[0] = playerAddress.blocks[2];
        BuildRotMatrixY(&objectRotation, scratch[7]);
        BuildRotMatrixX(&matrixWork, scratch[6]);
        MulMatrix2(&matrixWork, &objectRotation);
        BuildRotMatrixZ(&matrixWork, scratch[8]);
        MulMatrix2(&matrixWork, &objectRotation);
        cameraLift[0] = 0;
        cameraLift[1] = -0x1C0;
        cameraLift[2] = 0;
        TransposeMatrix(&objectRotation, &matrixWork);
        ApplyMatrix(&matrixWork, &cameraLift[0], &cameraLiftWorld[0]);
        scratch[2] += cameraLiftWorld[0] >> 4;
        scratch[3] += cameraLiftWorld[1] >> 4;
        scratch[4] += cameraLiftWorld[2] >> 4;
        scratch[6] += car->tiltCounter;
        g_CameraModePrev = 0;
        break;
    case 1:
        playerAddress.renderObject = car;
        scratchAddress.words = &scratch[2];
        scratchAddress.blocks[0] = playerAddress.blocks[0];
        chaseYawDamping = car->angleY;
        chaseTargetYaw = chaseYawDamping & 0xFFF;
        chaseCarSpeed = car->speed;
        g_ChaseCarSpeed = chaseCarSpeed;
        previousMode = g_CameraModePrev;
        g_ChaseTargetYaw = chaseTargetYaw;
        if (previousMode == 1) {
            modeAngle = &g_ChaseYawPrev;
            *modeAngle &= 0xFFF;
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
        g_ChaseYawDamping = ((((g_ChaseYawDamping * 6 * speedDamping) / 2500) - ((speedDamping * 0x46) / 50)) + 0xE0) / 10;
        }
        yawError = g_ChaseTargetYaw - g_ChaseYawPrev;
        if (yawError >= 5) {
            if (yawError >= 0x800) {
                chaseYawStepLimit = (((0x1000 - yawError) / 17) * 2) & 0xFFF;
                g_ChaseYawStepLimit = chaseYawStepLimit;
                if (chaseYawStepLimit >= 0x41) {
                    g_ChaseYawStepLimit = 0x40;
                }
                turnFactor = g_ChaseYawDamping;
                turnAccel = ((g_ChaseYawRampNeg + 8) * (g_ChaseYawRampNeg + 8)) / turnFactor;
                turnLimit = g_ChaseYawStepLimit;
                g_ChaseYawRampPos = 0;
                g_ChaseYawRampNeg += 8;
                g_ChaseYawStep = turnAccel;
                SettleChaseYaw(turnLimit, turnAccel, turnFactor, 1);
            } else {
                yawStepAhead =
                    (((g_ChaseTargetYaw - g_ChaseYawPrev) / 17) * 2) & 0xFFF;
                g_ChaseYawStepLimit = yawStepAhead;
                if (yawStepAhead >= 0x41) {
                    g_ChaseYawStepLimit = 0x40;
                }
                turnFactor = g_ChaseYawDamping;
                turnAccel = ((g_ChaseYawRampPos + 8) * (g_ChaseYawRampPos + 8)) /
                            turnFactor;
                turnLimit = g_ChaseYawStepLimit;
                g_ChaseYawRampNeg = 0;
                g_ChaseYawRampPos += 8;
                g_ChaseYawStep = turnAccel;
                SettleChaseYaw(turnLimit, turnAccel, turnFactor, 0);
            }
        } else if (yawError < -4) {
            if (yawError < -0x7FF) {
                yawStepWrapped = (((0x1000 - (g_ChaseYawPrev - g_ChaseTargetYaw)) / 17) * 2) & 0xFFF;
                g_ChaseYawStepLimit = yawStepWrapped;
                if (yawStepWrapped >= 0x41) {
                    g_ChaseYawStepLimit = 0x40;
                }
                turnFactor = g_ChaseYawDamping;
                turnAccel = ((g_ChaseYawRampPos + 8) * (g_ChaseYawRampPos + 8)) / turnFactor;
                turnLimit = g_ChaseYawStepLimit;
                g_ChaseYawRampNeg = 0;
                g_ChaseYawRampPos += 8;
                g_ChaseYawStep = turnAccel;
                SettleChaseYaw(turnLimit, turnAccel, turnFactor, 0);
            } else {
                yawStepBehind = (((g_ChaseYawPrev - g_ChaseTargetYaw) / 17) * 2) & 0xFFF;
                g_ChaseYawStepLimit = yawStepBehind;
                if (yawStepBehind >= 0x41) {
                    g_ChaseYawStepLimit = 0x40;
                }
                turnFactor = g_ChaseYawDamping;
                turnAccel = ((g_ChaseYawRampNeg + 8) * (g_ChaseYawRampNeg + 8)) / turnFactor;
                turnLimit = g_ChaseYawStepLimit;
                g_ChaseYawRampPos = 0;
                g_ChaseYawRampNeg += 8;
                g_ChaseYawStep = turnAccel;
                SettleChaseYaw(turnLimit, turnAccel, turnFactor, 1);
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
        scratch[2] += focusWorld[0];
        scratch[3] += focusWorld[1];
        scratch[4] += focusWorld[2];
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
        scratch[2] -= eyeWorld[0];
        scratch[3] -= eyeWorld[1];
        scratch[4] -= eyeWorld[2];
        chaseDistance = SquareRoot0((eyeWorld[0] * eyeWorld[0]) +
                                    (eyeWorld[2] * eyeWorld[2]));
        scratch[6] = 0x400 - (Atan2(eyeWorld[1] + 0x28, chaseDistance) & 0xFFF);
        scratch[7] = 0x400 - (Atan2(eyeWorld[0], eyeWorld[2]) & 0xFFF);
        scratch[7] += ChaseCameraYawOffset(car->steeringAngle);
        scratch[8] = car->bodyRoll - car->bodyRollVelocity;
        if (g_ChaseCameraPreset == 0) {
            negatedAccel = scratch[6] - 0x90;
        } else {
            negatedAccel = scratch[6] - 0x60;
        }
        scratch[6] = negatedAccel;
        g_CameraModePrev = 1;
        break;
    case 2:
        chaseNode = &g_TrackCameras[cameraNodeIndex];
        scratchAddress.words = &scratch[2];
        scratchAddress.blocks[0] = chaseNode->data.block;
        BuildRotMatrixY(&objectRotation, car->angleY);
        BuildRotMatrixX(&matrixWork, car->bodyPitch);
        MulMatrix2(&matrixWork, &objectRotation);
        BuildRotMatrixZ(&matrixWork, car->bodyRoll);
        MulMatrix2(&matrixWork, &objectRotation);
        TransposeMatrix(&objectRotation, &inverseObjectRotation);
        /* The point on the car the node looks at, in the car's frame and
         * then in the world. */
        nodeOffset[0] = chaseNode->offset[0];
        nodeOffset[1] = chaseNode->offset[1];
        nodeOffset[2] = chaseNode->offset[2] + 0x32;
        ApplyMatrixLV(&inverseObjectRotation, &nodeOffset[0],
                      &nodeWorld[0]);
        focusX = car->x + nodeWorld[0];
        focusY = car->y + nodeWorld[1];
        focusZ = car->z + nodeWorld[2];
        /* Pull the node's camera towards that point by the node's own blend,
         * then aim from where it ended up. */
        blend = chaseNode->data.world.blend;
        scratch[2] -= ((scratch[2] - focusX) * blend) / 10000;
        scratch[3] -= ((scratch[3] - focusY) * blend) / 10000;
        scratch[4] -= ((scratch[4] - focusZ) * blend) / 10000;
        AimCameraAt(scratch, focusX, focusY, focusZ);
        g_CameraModePrev = 2;
        break;
    case 3:
        playerAddress.renderObject = car;
        scratchAddress.words = &scratch[2];
        scratchAddress.blocks[0] = playerAddress.blocks[0];
        if (((u8)nodeChanged) || (g_CameraModePrev != 3)) {
            g_CamPathNode = cameraNodeIndex;
            g_CamPathFrame = 0;
            if (g_CameraModePrev == 3) {
                g_CamPathOffsetStart[0] = g_CamPathOffset[0];
                g_CamPathOffsetStart[1] = g_CamPathOffset[1];
                g_CamPathOffsetStart[2] = g_CamPathOffset[2];
                g_CamPathAngleStart[CAMPATH_PITCH] = g_CamPathAngle[CAMPATH_PITCH];
                g_CamPathAngleStart[CAMPATH_YAW] = g_CamPathAngle[CAMPATH_YAW];
                g_CamPathAngleStart[CAMPATH_ROLL] = g_CamPathAngle[CAMPATH_ROLL];
                g_CamPathAngleStart[CAMPATH_DIST] = g_CamPathAngle[CAMPATH_DIST];
            } else {
                prevNode = &g_TrackCameras[cameraNodeIndex];
                g_CamPathOffsetStart[0] = prevNode->offset[0];
                g_CamPathOffsetStart[1] = prevNode->offset[1];
                g_CamPathOffsetStart[2] = prevNode->offset[2];
                g_CamPathAngleStart[CAMPATH_PITCH] = prevNode->data.orientation.pitch;
                g_CamPathAngleStart[CAMPATH_YAW] = prevNode->data.orientation.yaw;
                g_CamPathAngleStart[CAMPATH_ROLL] = prevNode->data.orientation.roll;
                g_CamPathAngleStart[CAMPATH_DIST] = prevNode->data.orientation.distance;
            }
            pathNode = &g_TrackCameras[g_CamPathNode];
            g_CamPathOffsetDelta[0] = pathNode->offset[0] - g_CamPathOffsetStart[0];
            g_CamPathOffsetDelta[1] = pathNode->offset[1] - g_CamPathOffsetStart[1];
            g_CamPathOffsetDelta[2] = pathNode->offset[2] - g_CamPathOffsetStart[2];
            pitchDelta = pathNode->data.orientation.pitch - g_CamPathAngleStart[CAMPATH_PITCH];
            pitchDeltaPtr = &g_CamPathAngleDelta[CAMPATH_PITCH];
            *pitchDeltaPtr = pitchDelta;
            g_CamPathAngleDelta[CAMPATH_YAW] = pathNode->data.orientation.yaw - g_CamPathAngleStart[CAMPATH_YAW];
            g_CamPathAngleDelta[CAMPATH_ROLL] = pathNode->data.orientation.roll - g_CamPathAngleStart[CAMPATH_ROLL];
            g_CamPathAngleDelta[CAMPATH_DIST] = pathNode->data.orientation.distance - g_CamPathAngleStart[CAMPATH_DIST];
            if (pitchDelta > 0) {
                if (pitchDelta >= 0x800) {
                    *pitchDeltaPtr = pitchDelta - 0x1000;
                }
            } else if (pitchDelta < -0x7FF) {
                *pitchDeltaPtr = pitchDelta + 0x1000;
            }
            case3Angle = &g_CamPathAngleDelta[CAMPATH_YAW];
            if (*case3Angle > 0) {
                if (*case3Angle >= 0x800) {
                    *case3Angle -= 0x1000;
                }
            } else if (*case3Angle < -0x7FF) {
                *case3Angle += 0x1000;
            }
            case3Angle = &g_CamPathAngleDelta[CAMPATH_ROLL];
            if (*case3Angle > 0) {
                if (*case3Angle >= 0x800) {
                    *case3Angle -= 0x1000;
                }
            } else if (*case3Angle < -0x7FF) {
                *case3Angle += 0x1000;
            }
        } else if (g_CamPathFrame < g_TrackCameras[g_CamPathNode].duration) {
            g_CamPathFrame += 1;
        }
        pathBlend = 0x1000 - rcos((g_CamPathFrame << 0xB) / g_TrackCameras[g_CamPathNode].duration);
        offsetXProduct = pathBlend * g_CamPathOffsetDelta[0];
        if (offsetXProduct < 0) {
            offsetXProduct += 0x1FFF;
        }
        camPathOffset = (offsetXProduct >> 0xD) + g_CamPathOffsetStart[0];
        offsetYProduct = pathBlend * g_CamPathOffsetDelta[1];
        focusOffset[0] = camPathOffset;
        if (offsetYProduct < 0) {
            offsetYProduct += 0x1FFF;
        }
        pathOffsetY = (offsetYProduct >> 0xD) + g_CamPathOffsetStart[1];
        offsetZProduct = pathBlend * g_CamPathOffsetDelta[2];
        focusOffset[1] = pathOffsetY;
        if (offsetZProduct < 0) {
            offsetZProduct += 0x1FFF;
        }
        pathOffsetZ = (offsetZProduct >> 0xD) + g_CamPathOffsetStart[2];
        pitchProduct = pathBlend * g_CamPathAngleDelta[CAMPATH_PITCH];
        focusOffset[2] = pathOffsetZ;
        if (pitchProduct < 0) {
            pitchProduct += 0x1FFF;
        }
        pathPitch = (pitchProduct >> 0xD) + g_CamPathAngleStart[CAMPATH_PITCH];
        yawProduct = pathBlend * g_CamPathAngleDelta[CAMPATH_YAW];
        if (yawProduct < 0) {
            yawProduct += 0x1FFF;
        }
        pathYaw = (yawProduct >> 0xD) + g_CamPathAngleStart[CAMPATH_YAW];
        rollProduct = pathBlend * g_CamPathAngleDelta[CAMPATH_ROLL];
        if (rollProduct < 0) {
            rollProduct += 0x1FFF;
        }
        pathRoll = (rollProduct >> 0xD) + g_CamPathAngleStart[CAMPATH_ROLL];
        distProduct = pathBlend * g_CamPathAngleDelta[CAMPATH_DIST];
        if (distProduct < 0) {
            distProduct += 0x1FFF;
        }
        g_CamPathAngle[CAMPATH_PITCH] = pathPitch & 0xFFF;
        g_CamPathAngle[CAMPATH_YAW] = pathYaw & 0xFFF;
        g_CamPathAngle[CAMPATH_ROLL] = pathRoll & 0xFFF;
        g_CamPathOffset[0] = camPathOffset;
        g_CamPathOffset[1] = pathOffsetY;
        g_CamPathOffset[2] = pathOffsetZ;
        camPathAngle = (distProduct >> 0xD) + g_CamPathAngleStart[CAMPATH_DIST];
        g_CamPathAngle[CAMPATH_DIST] = camPathAngle;
        pathYawRelative = pathYaw - car->angleY;
        BuildRotMatrixY(&cameraRotation, pathYawRelative);
        BuildRotMatrixX(&matrixWork, pathPitch);
        MulMatrix2(&matrixWork, &cameraRotation);
        BuildRotMatrixZ(&matrixWork, pathRoll);
        MulMatrix2(&matrixWork, &cameraRotation);
        BuildRotMatrixY(&objectRotation, car->angleY);
        BuildRotMatrixX(&matrixWork, car->bodyPitch);
        MulMatrix2(&matrixWork, &objectRotation);
        BuildRotMatrixZ(&matrixWork, car->bodyRoll);
        MulMatrix2(&matrixWork, &objectRotation);
        TransposeMatrix(&objectRotation, &inverseObjectRotation);
        MulMatrix2(&cameraRotation, &objectRotation);
        TransposeMatrix(&objectRotation, &matrixWork);
        focusOffset[2] += 0x32;
        ApplyMatrixLV(&inverseObjectRotation, &focusOffset[0],
                      &focusWorld[0]);
        focusX = scratch[2] + focusWorld[0];
        focusY = scratch[3] + focusWorld[1];
        focusZ = scratch[4] + focusWorld[2];
        /* Sit the path's distance behind the focus point, then look back at
         * it. */
        eyeOffset[0] = 0;
        eyeOffset[1] = 0;
        eyeOffset[2] = g_CamPathAngle[CAMPATH_DIST];
        ApplyMatrixLV(&matrixWork, &eyeOffset[0], &eyeWorld[0]);
        scratch[2] = focusX - eyeWorld[0];
        scratch[3] = focusY - eyeWorld[1];
        scratch[4] = focusZ - eyeWorld[2];
        AimCameraAt(scratch, focusX, focusY, focusZ);
        /* Roll: take the camera's own right-hand axis back through the view
         * matrix and read how far off level it lands. */
        rollProbe[0] = 0x1000;
        rollProbe[1] = 0;
        rollProbe[2] = 0;
        BuildRotMatrixY(&cameraRotation, 0 - scratch[7]);
        ApplyMatrixLV(&cameraRotation, &rollProbe[0], &rollWork[0]);
        TransposeMatrix(&matrixWork, &cameraRotation);
        ApplyMatrixLV(&cameraRotation, &rollWork[0], &rollProbe[0]);
        scratch[8] = 0x400 - (Atan2(rollProbe[1], rollProbe[0]) & 0xFFF);
        g_CameraModePrev = 3;
        break;
    case 4:
        scratchAddress.words = &scratch[2];
        scratchAddress.blocks[0] = g_TrackCameras[cameraNodeIndex].data.block;
        if (((u8)nodeChanged) || (g_CameraModePrev != 4)) {
            g_CamPathFrame = 0;
        } else if (g_CamPathFrame < g_TrackCameras[cameraNodeIndex].duration) {
            g_CamPathFrame += 1;
        }
        BuildRotMatrixY(&objectRotation, car->angleY);
        BuildRotMatrixX(&matrixWork, car->bodyPitch);
        MulMatrix2(&matrixWork, &objectRotation);
        BuildRotMatrixZ(&matrixWork, car->bodyRoll);
        MulMatrix2(&matrixWork, &objectRotation);
        TransposeMatrix(&objectRotation, &inverseObjectRotation);
        orbitNode = &g_TrackCameras[cameraNodeIndex];
        nodeOffset[0] = 0;
        nodeOffset[1] = orbitNode->data.orientation.distance;
        nodeOffset[2] = 0x32;
        ApplyMatrixLV(&inverseObjectRotation, &nodeOffset[0],
                      &nodeWorld[0]);
        /* Slide the camera from where it starts to the node's own position
         * across the node's duration, then aim back at the car. */
        scratch[2] += ((orbitNode->offset[0] - scratch[2]) * g_CamPathFrame) /
                      orbitNode->duration;
        scratch[3] += ((orbitNode->offset[1] - scratch[3]) * g_CamPathFrame) /
                      orbitNode->duration;
        scratch[4] += ((orbitNode->offset[2] - scratch[4]) * g_CamPathFrame) /
                      orbitNode->duration;
        AimCameraAt(scratch, car->x + nodeWorld[0], car->y + nodeWorld[1],
                    car->z + nodeWorld[2]);
        g_CameraModePrev = 4;
        break;
    case 5:
        playerAddress.renderObject = car;
        scratchAddress.words = &scratch[2];
        scratchAddress.blocks[0] = playerAddress.blocks[0];
        BuildRotMatrixY(&cameraRotation, 0 - g_OrbitCameraYaw);
        BuildRotMatrixY(&objectRotation, car->angleY);
        BuildRotMatrixX(&matrixWork, car->bodyPitch);
        MulMatrix2(&matrixWork, &objectRotation);
        BuildRotMatrixZ(&matrixWork, car->bodyRoll);
        MulMatrix2(&matrixWork, &objectRotation);
        TransposeMatrix(&objectRotation, &inverseObjectRotation);
        MulMatrix2(&cameraRotation, &objectRotation);
        TransposeMatrix(&objectRotation, &matrixWork);
        focusOffset[0] = 0;
        focusOffset[1] = 0;
        focusOffset[2] = 0x32;
        ApplyMatrixLV(&inverseObjectRotation, &focusOffset[0],
                      &focusWorld[0]);
        scratch[2] += focusWorld[0];
        scratch[3] += focusWorld[1];
        scratch[4] += focusWorld[2];
        eyeOffset[0] = 0;
        eyeOffset[1] = 0;
        eyeOffset[2] = g_OrbitCameraDistance;
        ApplyMatrixLV(&matrixWork, &eyeOffset[0], &eyeWorld[0]);
        /* Pitch uses the orbit distance rather than the flattened eye vector,
         * so a pitched camera tilts a shade less than a true look-at would.
         * Retail's, and the view players know. */
        scratch[6] = 0x400 - (Atan2(eyeWorld[1], g_OrbitCameraDistance) & 0xFFF);
        scratch[7] = 0x400 - (Atan2(eyeWorld[0], eyeWorld[2]) & 0xFFF);
        scratch[8] = car->bodyRoll;
        g_CameraModePrev = 5;
        scratch[2] -= eyeWorld[0];
        scratch[3] = (scratch[3] - 0x28) - eyeWorld[1];
        scratch[4] -= eyeWorld[2];
        break;
    }
    StoreScratchLegacyView(&legacyView);
    SetCameraRotMatrix();
    if (cameraModeSel > 0) {
        playerAddress.player = &g_PlayerCar;
        if (car == playerAddress.renderObject) {
            SelectModelBank(0);
            DrawPlayerCarModel(car);
        }
    }
}

/* Deliberately raw: the environment script header word 0 is stored here and
 * never read anywhere in the image. */

void SetEnvironmentScript(u32 *script) {
    GameEnvironmentScriptAddress address;
    u32 value0;
    u32 value1;

    value0 = *script;
    script++;
    g_SkyRowBase = value0;
    value1 = *script;
    script++;
    address.words = script;
    g_EnvScriptCues = address.cues;
    g_EnvScriptLength = value1;
}
