#include "game/track_internal.h"
#include "game/prim.h"
#include "game/audio.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"

s32 IsCarNearWaypoint(TrackWaypointRuntime *waypoint) {
    return waypoint->motion.x > g_PlayerCar.x - 0x40 &&
           waypoint->motion.x < g_PlayerCar.x + 0x40 &&
           waypoint->motion.y > g_PlayerCar.z - 0x40 &&
           waypoint->motion.y < g_PlayerCar.z + 0x40;
}

void UpdateWaypoints(void) {
    TrackWaypointRuntime *waypoint;
    s32 i;

    if (g_WaypointSpawnCooldown != 0) {
        g_WaypointSpawnCooldown--;
    }

    waypoint = g_Waypoints;
    i = 0;
    do {
        if (waypoint->active == 0) {
            if (IsCarNearWaypoint(waypoint) != 0) {
                g_WaypointsCollected++;
                PlaySoundCue(0xA);

                waypoint->active = 1;
                waypoint->motion.velocity.vector = g_PlayerVelocity[0];

                waypoint->motion.velocity.fields.x *= 2;
                waypoint->motion.velocity.fields.y *= 2;
                waypoint->motion.velocityMagnitude =
                    ((waypoint->motion.velocity.fields.x * waypoint->motion.velocity.fields.x) + (waypoint->motion.velocity.fields.y * waypoint->motion.velocity.fields.y)) /
                    0x2000;
            }
        } else if (waypoint->active == 1) {
            waypoint->motion.x += waypoint->motion.velocity.fields.x / 0x100;
            waypoint->motion.y += waypoint->motion.velocity.fields.y / 0x100;
            waypoint->motion.velocity.fields.x = (waypoint->motion.velocity.fields.x * 15) / 16;
            waypoint->motion.velocity.fields.y = (waypoint->motion.velocity.fields.y * 15) / 16;
            waypoint->motion.rotationY += waypoint->motion.velocityMagnitude / 0x100;
            waypoint->motion.velocityMagnitude = (waypoint->motion.velocityMagnitude * 15) / 16;

            if (waypoint->motion.rotationZ < 0x400) {
                waypoint->motion.rotationZ += 0x80;
            } else {
                waypoint->motion.rotationZ = 0x400;
            }

            if ((waypoint->motion.velocity.fields.x == 0) && (waypoint->motion.velocity.fields.y == 0) && (waypoint->motion.velocityMagnitude == 0)) {
                waypoint->active = 2;
            }
        }

        i++;
        waypoint++;
    } while (i < 6);
}

/*
 * Renders the 6 waypoints. For each active-shaped slot it builds a rotation
 * matrix from the waypoint's Y and Z rotations and emits
 * two GTE draw primitives (SubmitModel) into the render state's OT: the second is
 * the same billboard rotated by 0x800 (180 degrees).
 */
void DrawWaypoints(void) {
    Matrix mtx0;
    Matrix mtx1;
    s32 drawId;
    s32 i;
    Matrix *mtx1Ptr;
    TrackWaypointRuntime *waypoint;
    s32 frameValue;
    s32 drawArg;

    drawId = 2;
    SelectModelBank(0);
    i = 0;
    mtx1Ptr = &mtx1;
    waypoint = g_Waypoints;

    do {
        BuildRotMatrixY(&mtx0, waypoint->motion.rotationY);
        MulMatrix2((&g_RenderState.matrix), &mtx0);
        BuildRotMatrixZ(mtx1Ptr, waypoint->motion.rotationZ);
        MulMatrix(&mtx0, mtx1Ptr);
        SetGteObjectMatrix((&g_ObjectMatrixWork),
                       AsPositionWords(&waypoint->motion.x), &mtx0);
        frameValue = g_ModelBankCount;
        g_RenderState.envMode4 = 0;
        drawArg = 1;
        if (drawId < frameValue) {
            drawArg = drawId;
        }
        SubmitModel((&g_RenderState), drawArg);

        BuildRotMatrixY(mtx1Ptr, 0x800);
        MulMatrix2(&mtx0, mtx1Ptr);
        SetGteObjectMatrix((&g_ObjectMatrixWork),
                       AsPositionWords(&waypoint->motion.x), mtx1Ptr);
        frameValue = g_ModelBankCount;
        g_RenderState.envMode4 = 0;
        drawArg = 1;
        if (drawId < frameValue) {
            drawArg = drawId;
        }
        SubmitModel((&g_RenderState), drawArg);

        i++;
        waypoint++;
    } while (i < 6);
}

void DrawLapNumber(void) {
    SPRT *cursor;
    s32 track;
    s32 divisor;
    s32 digitsDrawn;
    s32 xOffset;
    s32 quotient;
    SPRT *packet;

    cursor = RENDER_PRIM_CURSOR_AS(SPRT);
    track = g_PlayerCar.lap;
    divisor = 1;
    digitsDrawn = 0;
    xOffset = 0;
    packet = cursor;

    while (1) {
        quotient = track / divisor;
        if (quotient == 0 && digitsDrawn > 0) {
            break;
        }

        {
            s32 y;
            SPRT *oldPacket;
            s32 tens;

            SetSprt(cursor);
            SetShadeTex(cursor, 1);

            y = 0x120 - xOffset;
            oldPacket = packet;
            tens = quotient / 10;
            divisor *= 10;
            xOffset += 0x18;
            digitsDrawn++;
            cursor++;
            packet->v0 = 0x48;
            packet->w = 0x18;
            packet->h = 0x20;
            packet->y0 = 0x10;
            packet->clut = 0x780B;
            packet->x0 = y;
            packet->u0 = (quotient - tens * 10) * 24;

            packet++;
            AddPrim(GamePrimaryOrderingTable(0), oldPacket);
        }
    }

    {
        /*
         * The digits are drawn with a texture page of their own, queued ahead
         * of them. The recovered code stored the queue's answer into the
         * render state twice over, at a slot nothing ever read back; only
         * the queueing itself does anything.
         */
        RenderBufferAddress packetAddress;

        packetAddress.sprite = cursor;
        QueueDrawModePrim(GamePrimaryOrderingTable(0), packetAddress.bytes, 9);
    }
}

void InitRivalCar(GameCarRuntime *ent, s32 pos, RaceGridSlot *slots) {
    const TrackRivalStart *start =
        &g_TrackEventData->rivalStarts[g_RaceSeries][pos + 1];
    CarTrackLimits trackLimits = {
        .rightInset = 20,
        .leftInset = -20,
    };
    s32 trackPointIndex;

    ent->initializedFlag = 1;
    ent->collisionFlag = 0;
    ent->aiEnabled = 1;
    ent->modelIndex = slots[pos].halves.modelId;
    ent->rivalModelId = slots[pos].halves.modelId;
    ent->trackPointIndex = start->trackPointIndex;
    ent->x = start->x;
    ent->z = start->z;
    ent->y = 0;

    trackPointIndex = FindTrackSegment(ent, ent->trackPointIndex);
    ent->trackPointIndex = trackPointIndex;
    ent->bodyPitch = 0;
    ent->bodyYaw =
        (0xC00 - (g_RaceSeries << 11) - TrackPoint(trackPointIndex)->angle) &
        0xFFF;
    ent->bodyRoll = 0;
    ent->bodyRollVelocity = 0;
    ent->progressB = 0;
    ent->progressA = 0;
    ent->trackProgress = 0;
    ent->speed = 0;
    ent->acceleration = 0;
    ent->worldVelocityZ = 0;
    ent->reservedCC = 0;
    ent->worldVelocityX = 0;
    ent->reservedE0 = 0;
    ent->reservedDC = 0;
    ent->reservedD8 = 0;
    ent->motionZ = 0;
    ent->motionY = 0;
    ent->motionX = 0;
    ent->routeIndex = 0;
    ent->reserved116 = 0;
    ent->reserved110 = 0;
    ent->yawRate = 0;
    ent->routeMarkerActive = 0;
    ent->slideInput.value = 0;
    ent->baseBodyYaw = ent->bodyYaw;
    ent->targetYaw = ent->bodyYaw;
    ent->headingAngle = ent->bodyYaw;
    ent->reservedF8 = 0;
    ent->avoidanceActive = 0;
    ent->reservedC4 = 0;
    ent->routeMarkerIndex = 0;
    SeedCarLapProgress(ent, start->modelId);

    ent->activeFlag = start->modelId;
    if (start->modelId != -1) {
        UpdateCarTrackState(ent, ent->trackPointIndex, &trackLimits);
        ent->modelY = ent->y;
        ent->previousTrackProgress = ent->trackProgress;
    }

    ent->avoidanceStep = 0;
    ent->initialLateralOffset = ent->trackLateralOffset;
    ent->avoidanceTargetOffset = ent->trackLateralOffset;
    ent->aiLateralOffset = ent->trackLateralOffset;
    CopyCarBodyRotationToModel(ent);
    ent->reserved40 = 0;
    ent->steeringAngle = 0;
    ent->wheelRotation = 0;
    ent->modelY = ent->y;
}
