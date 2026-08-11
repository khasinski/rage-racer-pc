#include "common.h"
#include "game/render.h"
#include "game/scratchpad.h"
#include "game/track.h"
#include "game/track_internal.h"
#include "game/player_car_internal.h"
#include "game/vector.h"
#include "psyq/gte.h"

/* Mode-3 camera path: the eye is eased from one track-camera node to the next
 * over `node->duration` frames. Each of offset (a local xyz applied through the
 * car's matrix) and orientation (pitch/yaw/roll/distance) keeps a start value,
 * a delta to the destination and the current interpolated value. Mode-3 nodes
 * therefore store angles in the first four words where modes 2/4 store a world
 * position -- the record is a union keyed on `node->mode`. */
/* The orientation quads, same three-group shape as the offsets: delta at
 * 0x8009B1E8, start at +0x10, current at +0x20. Elements 0..2 are pitch, yaw
 * and roll -- 12-bit angles, wrapped to +-0x800 on load and masked with 0xFFF
 * on store -- and element 3 is the pull-back distance, a plain length. */
#define CAMPATH_PITCH 0
#define CAMPATH_YAW 1
#define CAMPATH_ROLL 2
#define CAMPATH_DIST 3

typedef union CameraMatrixBuffer {
    Matrix matrix;
    u16 halfwords[16];
    s32 words[8];
} CameraMatrixBuffer;

typedef union CameraCarAddress {
    PlayerCarRuntime *player;
    GameRenderObject *renderObject;
    Block16 *blocks;
} CameraCarAddress;

void UpdateCamera(CameraViewMode cameraModeSel, GameRenderObject *car) {
    s16 sp10[4];
    s32 sp18[3];
    s32 sp28[3];
    s32 sp38[4];
    CameraMatrixBuffer objectRotation;
    CameraMatrixBuffer matrixWork;
    CameraMatrixBuffer cameraRotation;
    CameraMatrixBuffer inverseObjectRotation;
    s32 previousMode;
    s32 rawAngle;
    s32 adjustedY;
    s32 turnLimit;
    s32 turnAccel;
    s32 turnFactor;
    s32 *angleState;
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
    s32 toCarX;
    s32 pathYaw;
    s32 pathPitch;
    s32 squaredX;
    s32 squaredZ;
    s32 orbitNodeOffset;
    s32 chaseNodeOffset;
    s32 pathOffsetZ;
    s32 pathOffsetY;
    s32 camPathOffset;
    s32 camPathAngle;
    s32 pathDistance;
    s32 orbitToCarX;
    s32 orbitToCarY;
    s32 orbitToCarZ;
    s32 orbitDx;
    s32 orbitDz;
    s32 chaseYawDamping;
    s32 chaseYawStepLimit;
    s32 cameraNodeIndex;
    s32 yawStepAhead;
    s32 yawStepWrapped;
    s32 yawStepBehind;
    s32 chaseYawLag;
    s32 chaseDistance;
    s32 chaseDx;
    s32 chaseDz;
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
    s32 chaseYaw;
    s32 screenWidth;
    s32 negatedAccel;
    s32 wrappedLag;
    u8 nextPrevMode;
    GameTrackCameraNode *pathNode;
    GameTrackCameraNode *orbitNode;
    GameTrackCameraNode *chaseNodeAgain;
    GameTrackCameraNode *chaseNode;
    GameTrackCameraNode *chaseNodeOffsets;
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
        BuildRotMatrixY(objectRotation.halfwords, scratch[7]);
        BuildRotMatrixX(matrixWork.halfwords, scratch[6]);
        MulMatrix2(matrixWork.halfwords, objectRotation.halfwords);
        BuildRotMatrixZ(matrixWork.halfwords, scratch[8]);
        MulMatrix2(matrixWork.halfwords, objectRotation.halfwords);
        sp10[0] = 0;
        sp10[1] = -0x1C0;
        sp10[2] = 0;
        matrixWork.halfwords[0] = objectRotation.halfwords[0];
        matrixWork.halfwords[1] = objectRotation.halfwords[3];
        matrixWork.halfwords[2] = objectRotation.halfwords[6];
        matrixWork.halfwords[3] = objectRotation.halfwords[1];
        matrixWork.halfwords[4] = objectRotation.halfwords[4];
        matrixWork.halfwords[5] = objectRotation.halfwords[7];
        matrixWork.halfwords[6] = objectRotation.halfwords[2];
        matrixWork.halfwords[7] = objectRotation.halfwords[5];
        matrixWork.halfwords[8] = objectRotation.halfwords[8];
        ApplyMatrix(matrixWork.halfwords, &sp10[0], &sp38[0]);
        scratch[2] += sp38[0] >> 4;
        scratch[3] += sp38[1] >> 4;
        scratch[4] += sp38[2] >> 4;
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
                if (turnLimit >= turnAccel) {
                    negatedAccel = 0 - turnAccel;
                    goto block_36;
                }
                goto block_34;
            }
            yawStepAhead = (((g_ChaseTargetYaw - g_ChaseYawPrev) / 17) * 2) & 0xFFF;
            g_ChaseYawStepLimit = yawStepAhead;
            if (yawStepAhead >= 0x41) {
                g_ChaseYawStepLimit = 0x40;
            }
            turnFactor = g_ChaseYawDamping;
            turnAccel = ((g_ChaseYawRampPos + 8) * (g_ChaseYawRampPos + 8)) / turnFactor;
            turnLimit = g_ChaseYawStepLimit;
            g_ChaseYawRampNeg = 0;
            g_ChaseYawRampPos += 8;
            g_ChaseYawStep = turnAccel;
            if (turnLimit < turnAccel) {
                goto block_29;
            }
            goto block_30;
        }
        if (yawError < -4) {
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
                if (turnLimit < turnAccel) {
block_29:
                    g_ChaseYawLag = turnLimit;
                    g_ChaseYawRampPos = SquareRoot0(turnLimit * turnFactor);
                } else {
block_30:
                    g_ChaseYawLag = turnAccel;
                }
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
                if (turnLimit < turnAccel) {
block_34:
                    g_ChaseYawLag = 0 - turnLimit;
                    g_ChaseYawRampNeg = SquareRoot0(turnLimit * turnFactor);
                } else {
                    negatedAccel = 0 - turnAccel;
block_36:
                    g_ChaseYawLag = negatedAccel;
                }
            }
        } else {
            g_ChaseYawLag = 0;
            g_ChaseYawRampNeg = 0;
            g_ChaseYawRampPos = 0;
        }
        rawAngle = g_ChaseTargetYaw;
        chaseCarSpeed = (g_ChaseYawPrev + g_ChaseYawLag) & 0xFFF;
        g_ChaseYaw = chaseCarSpeed;
        if (rawAngle < chaseCarSpeed) {
            chaseYawLag = rawAngle - chaseCarSpeed;
            g_ChaseYawLag = chaseYawLag;
            wrappedLag = chaseYawLag;
            if (wrappedLag < -0x7FF) {
                wrappedLag += 0x1000;
            }
            angleState = &g_ChaseYawLag;
            *angleState = wrappedLag;
        } else {
            chaseYawLag = rawAngle - chaseCarSpeed;
            g_ChaseYawLag = chaseYawLag;
            wrappedLag = chaseYawLag;
            if (wrappedLag >= 0x800) {
                wrappedLag -= 0x1000;
            }
            angleState = &g_ChaseYawLag;
            *angleState = wrappedLag;
        }
        angleState = cameraRotation.words;
        BuildRotMatrixY(angleState, 0 - g_ChaseYawLag);
        BuildRotMatrixX(matrixWork.halfwords, -0x80);
        MulMatrix2(matrixWork.halfwords, cameraRotation.halfwords);
        {
            volatile s32 *angleY = &car->angleY;

            g_ChaseYawPrev = g_ChaseYaw;
            BuildRotMatrixY(objectRotation.halfwords, *angleY);
        }
        BuildRotMatrixX(matrixWork.halfwords, car->bodyPitch);
        MulMatrix2(matrixWork.halfwords, objectRotation.halfwords);
        BuildRotMatrixZ(matrixWork.halfwords, car->bodyRoll);
        MulMatrix2(matrixWork.halfwords, objectRotation.halfwords);
        TransposeMatrix(&objectRotation.matrix, &inverseObjectRotation.matrix);
        MulMatrix2(angleState, objectRotation.halfwords);
        TransposeMatrix(&objectRotation.matrix, &matrixWork.matrix);
        sp18[1] = -0x3C;
        sp18[0] = 0;
        sp18[2] = 0x32;
        ApplyMatrixLV(inverseObjectRotation.halfwords, &sp18[0], &sp28[0]);
        sp18[0] = 0;
        scratch[2] += sp28[0];
        scratch[3] += sp28[1];
        scratch[4] += sp28[2];
        switch (g_ChaseCameraPreset) {
        case 0:
            sp18[1] = 0x3A;
            screenWidth = 0x118;
            goto block_52;
        case 1:
            sp18[1] = 0x59;
            screenWidth = 0x140;
            goto block_52;
        case 2:
            sp18[1] = 0x97;
            screenWidth = 0x190;
block_52:
            sp18[2] = screenWidth;
            break;
        }
        ApplyMatrixLV(matrixWork.halfwords, &sp18[0], &sp38[0]);
        scratch[2] -= sp38[0];
        scratch[3] -= sp38[1];
        scratch[4] -= sp38[2];
        chaseDistance = SquareRoot0((sp38[0] * sp38[0]) + (sp38[2] * sp38[2]));
        sp38[3] = chaseDistance;
        scratch[6] = 0x400 - (Atan2(sp38[1] + 0x28, chaseDistance) & 0xFFF);
        scratch[7] = 0x400 - (Atan2(sp38[0], sp38[2]) & 0xFFF);
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
        chaseNodeOffset = cameraNodeIndex * 0x24;
        chaseNode = &g_TrackCameras[cameraNodeIndex];
        scratchAddress.words = &scratch[2];
        scratchAddress.blocks[0] = chaseNode->data.block;
        BuildRotMatrixY(objectRotation.halfwords, car->angleY);
        BuildRotMatrixX(matrixWork.halfwords, car->bodyPitch);
        MulMatrix2(matrixWork.halfwords, objectRotation.halfwords);
        BuildRotMatrixZ(matrixWork.halfwords, car->bodyRoll);
        MulMatrix2(matrixWork.halfwords, objectRotation.halfwords);
        TransposeMatrix(&objectRotation.matrix, &inverseObjectRotation.matrix);
        chaseNodeOffsets = &g_TrackCameras[cameraNodeIndex];
        sp18[0] = chaseNodeOffsets->offset[0];
        sp18[1] = chaseNodeOffsets->offset[1];
        sp18[2] = chaseNodeOffsets->offset[2] + 0x32;
        ApplyMatrixLV(inverseObjectRotation.halfwords, &sp18[0], &sp28[0]);
        toCarX = scratch[2] - (car->x + sp28[0]);
        sp38[0] = toCarX;
        sp38[1] = scratch[3] - (car->y + sp28[1]);
        chaseNodeAgain = &g_TrackCameras[cameraNodeIndex];
        sp38[2] = scratch[4] - (car->z + sp28[2]);
        scratch[2] -= (toCarX * chaseNodeAgain->data.world.blend) / 10000;
        scratch[3] -= (sp38[1] * chaseNodeAgain->data.world.blend) / 10000;
        scratch[4] -= (sp38[2] * chaseNodeAgain->data.world.blend) / 10000;
        chaseDx = scratch[2] - (car->x + sp28[0]);
        squaredX = chaseDx * chaseDx;
        sp38[0] = chaseDx;
        sp38[1] = scratch[3] - (car->y + sp28[1]);
        chaseDz = scratch[4] - (car->z + sp28[2]);
        sp38[2] = chaseDz;
        squaredZ = chaseDz * chaseDz;
        scratch[6] = 0x400 - (Atan2(0 - sp38[1], SquareRoot0(squaredX + squaredZ)) & 0xFFF);
        chaseYaw = 0x400 - (Atan2(0 - sp38[0], 0 - sp38[2]) & 0xFFF);
        nextPrevMode = 2;
        scratch[7] = chaseYaw;
        scratch[8] = 0;
        g_CameraModePrev = nextPrevMode;
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
        sp18[0] = camPathOffset;
        if (offsetYProduct < 0) {
            offsetYProduct += 0x1FFF;
        }
        pathOffsetY = (offsetYProduct >> 0xD) + g_CamPathOffsetStart[1];
        offsetZProduct = pathBlend * g_CamPathOffsetDelta[2];
        sp18[1] = pathOffsetY;
        if (offsetZProduct < 0) {
            offsetZProduct += 0x1FFF;
        }
        pathOffsetZ = (offsetZProduct >> 0xD) + g_CamPathOffsetStart[2];
        pitchProduct = pathBlend * g_CamPathAngleDelta[CAMPATH_PITCH];
        sp18[2] = pathOffsetZ;
        if (pitchProduct < 0) {
            pitchProduct += 0x1FFF;
        }
        pathPitch = (pitchProduct >> 0xD) + g_CamPathAngleStart[CAMPATH_PITCH];
        yawProduct = pathBlend * g_CamPathAngleDelta[CAMPATH_YAW];
        sp38[0] = pathPitch;
        if (yawProduct < 0) {
            yawProduct += 0x1FFF;
        }
        pathYaw = (yawProduct >> 0xD) + g_CamPathAngleStart[CAMPATH_YAW];
        rollProduct = pathBlend * g_CamPathAngleDelta[CAMPATH_ROLL];
        sp38[1] = pathYaw;
        if (rollProduct < 0) {
            rollProduct += 0x1FFF;
        }
        pathRoll = (rollProduct >> 0xD) + g_CamPathAngleStart[CAMPATH_ROLL];
        distProduct = pathBlend * g_CamPathAngleDelta[CAMPATH_DIST];
        sp38[2] = pathRoll;
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
        sp38[3] = camPathAngle;
        g_CamPathAngle[CAMPATH_DIST] = camPathAngle;
        pathYawRelative = pathYaw - car->angleY;
        sp38[1] = pathYawRelative;
        BuildRotMatrixY(cameraRotation.halfwords, pathYawRelative);
        BuildRotMatrixX(matrixWork.halfwords, sp38[0]);
        MulMatrix2(matrixWork.halfwords, cameraRotation.halfwords);
        BuildRotMatrixZ(matrixWork.halfwords, sp38[2]);
        MulMatrix2(matrixWork.halfwords, cameraRotation.halfwords);
        BuildRotMatrixY(objectRotation.halfwords, car->angleY);
        BuildRotMatrixX(matrixWork.halfwords, car->bodyPitch);
        MulMatrix2(matrixWork.halfwords, objectRotation.halfwords);
        BuildRotMatrixZ(matrixWork.halfwords, car->bodyRoll);
        MulMatrix2(matrixWork.halfwords, objectRotation.halfwords);
        TransposeMatrix(&objectRotation.matrix, &inverseObjectRotation.matrix);
        MulMatrix2(cameraRotation.halfwords, objectRotation.halfwords);
        TransposeMatrix(&objectRotation.matrix, &matrixWork.matrix);
        sp18[2] += 0x32;
        ApplyMatrixLV(inverseObjectRotation.halfwords, &sp18[0], &sp38[0]);
        sp28[0] = 0;
        sp28[1] = 0;
        scratch[2] += sp38[0];
        scratch[3] += sp38[1];
        scratch[4] += sp38[2];
        sp28[2] = g_CamPathAngle[CAMPATH_DIST];
        ApplyMatrixLV(matrixWork.halfwords, &sp28[0], &sp38[0]);
        scratch[2] -= sp38[0];
        scratch[3] -= sp38[1];
        scratch[4] -= sp38[2];
        pathDistance = SquareRoot0((sp38[0] * sp38[0]) + (sp38[2] * sp38[2]));
        sp38[3] = pathDistance;
        scratch[6] = 0x400 - (Atan2(sp38[1], pathDistance) & 0xFFF);
        scratch[7] = 0x400 - (Atan2(sp38[0], sp38[2]) & 0xFFF);
        scratch[8] = 0;
        sp18[0] = 0x1000;
        sp18[1] = 0;
        sp18[2] = 0;
        BuildRotMatrixY(cameraRotation.halfwords, 0 - scratch[7]);
        ApplyMatrixLV(cameraRotation.halfwords, &sp18[0], &sp28[0]);
        TransposeMatrix(&matrixWork.matrix, &cameraRotation.matrix);
        ApplyMatrixLV(cameraRotation.halfwords, &sp28[0], &sp18[0]);
        scratch[8] = 0x400 - (Atan2(sp18[1], sp18[0]) & 0xFFF);
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
        BuildRotMatrixY(objectRotation.halfwords, car->angleY);
        BuildRotMatrixX(matrixWork.halfwords, car->bodyPitch);
        MulMatrix2(matrixWork.halfwords, objectRotation.halfwords);
        BuildRotMatrixZ(matrixWork.halfwords, car->bodyRoll);
        MulMatrix2(matrixWork.halfwords, objectRotation.halfwords);
        TransposeMatrix(&objectRotation.matrix, &inverseObjectRotation.matrix);
        sp18[0] = 0;
        orbitNodeOffset = cameraNodeIndex * 0x24;
        sp18[1] = g_TrackCameras[cameraNodeIndex].data.orientation.distance;
        sp18[2] = 0x32;
        ApplyMatrixLV(inverseObjectRotation.halfwords, &sp18[0], &sp28[0]);
        orbitNode = &g_TrackCameras[cameraNodeIndex];
        orbitToCarX = orbitNode->offset[0] - scratch[2];
        sp38[0] = orbitToCarX;
        orbitToCarY = orbitNode->offset[1] - scratch[3];
        sp38[1] = orbitToCarY;
        orbitToCarZ = orbitNode->offset[2] - scratch[4];
        sp38[2] = orbitToCarZ;
        scratch[2] += (orbitToCarX * g_CamPathFrame) / orbitNode->duration;
        scratch[3] += (sp38[1] * g_CamPathFrame) / orbitNode->duration;
        scratch[4] += (sp38[2] * g_CamPathFrame) / orbitNode->duration;
        orbitDx = scratch[2] - (car->x + sp28[0]);
        squaredX = orbitDx * orbitDx;
        sp38[0] = orbitDx;
        sp38[1] = scratch[3] - (car->y + sp28[1]);
        orbitDz = scratch[4] - (car->z + sp28[2]);
        sp38[2] = orbitDz;
        squaredZ = orbitDz * orbitDz;
        scratch[6] = 0x400 - (Atan2(0 - sp38[1], SquareRoot0(squaredX + squaredZ)) & 0xFFF);
        chaseYaw = 0x400 - (Atan2(0 - sp38[0], 0 - sp38[2]) & 0xFFF);
        nextPrevMode = 4;
        scratch[7] = chaseYaw;
        scratch[8] = 0;
        g_CameraModePrev = nextPrevMode;
        break;
    case 5:
        playerAddress.renderObject = car;
        scratchAddress.words = &scratch[2];
        scratchAddress.blocks[0] = playerAddress.blocks[0];
        BuildRotMatrixY(cameraRotation.halfwords, 0 - g_OrbitCameraYaw);
        BuildRotMatrixY(objectRotation.halfwords, car->angleY);
        BuildRotMatrixX(matrixWork.halfwords, car->bodyPitch);
        MulMatrix2(matrixWork.halfwords, objectRotation.halfwords);
        BuildRotMatrixZ(matrixWork.halfwords, car->bodyRoll);
        MulMatrix2(matrixWork.halfwords, objectRotation.halfwords);
        TransposeMatrix(&objectRotation.matrix, &inverseObjectRotation.matrix);
        MulMatrix2(cameraRotation.halfwords, objectRotation.halfwords);
        TransposeMatrix(&objectRotation.matrix, &matrixWork.matrix);
        sp18[0] = 0;
        sp18[1] = 0;
        sp18[2] = 0x32;
        ApplyMatrixLV(inverseObjectRotation.halfwords, &sp18[0], &sp28[0]);
        sp18[0] = 0;
        sp18[1] = 0;
        scratch[2] += sp28[0];
        scratch[3] += sp28[1];
        scratch[4] += sp28[2];
        sp18[2] = g_OrbitCameraDistance;
        ApplyMatrixLV(matrixWork.halfwords, &sp18[0], &sp38[0]);
        scratch[6] = 0x400 - (Atan2(sp38[1], g_OrbitCameraDistance) & 0xFFF);
        scratch[7] = 0x400 - (Atan2(sp38[0], sp38[2]) & 0xFFF);
        scratch[8] = car->bodyRoll;
        g_CameraModePrev = 5;
        scratch[2] -= sp38[0];
        adjustedY = scratch[3] - 0x28;
        scratch[3] = adjustedY - sp38[1];
        scratch[4] -= sp38[2];
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
 * never read anywhere in the image (docs/names.md 15g). */

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
