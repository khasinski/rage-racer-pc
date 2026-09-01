#include "game/track_internal.h"
#include "game/prim.h"
#include "game/audio.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"

typedef struct CarTrackLimitWork {
    s32 reserved[4];
    CarTrackLimits limits;
} CarTrackLimitWork;


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
    u8 *base;
    s32 sub;
    u8 *p;
    u16 val122;
    s32 scene;
    u16 av;
    TrackEventDataAddress eventAddress;

    ent->initializedFlag = 1;
    av = slots[pos].halves.modelId;
    sub = (pos + 1) * 12;
    {
        u8 *baseValue;
        eventAddress.pointer = g_TrackEventData;
        baseValue = eventAddress.bytePointer;
        base = baseValue;
    }
    ent->collisionFlag = 0;
    ent->aiEnabled = 1;
    ent->modelIndex = av;
    val122 = slots[pos].halves.modelId;
    scene = g_RaceSeries;
    ent->rivalModelId = val122;
    {
        TrackRivalStart *p1;

        eventAddress.bytePointer = base + (sub + scene * 144) + 0x354;
        p1 = eventAddress.rivalStart;
        ent->trackPointIndex = p1->trackPointIndex;
        ent->x = p1->x;
        ent->z = p1->z;
        ent->y = 0;
    }
    {
        s32 ret = FindTrackSegment(ent, ent->trackPointIndex);
        s32 lev = g_RaceSeries;
        s32 idx;
        s32 levShift;
        s32 acc;
        s32 angle;

        ent->trackPointIndex = ret;
        ent->bodyPitch = 0;
        idx = ent->trackPointIndex;
        acc = 0xC00;
        levShift = lev << 11;
        angle = TrackPoint(idx)->angle;
        acc -= levShift;
        ent->bodyYaw = (acc - angle) & 0xFFF;

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
        p = base + (sub + lev * 144);
        ent->targetYaw = ent->bodyYaw;
        ent->headingAngle = ent->bodyYaw;
        ent->reservedF8 = 0;
        ent->avoidanceActive = 0;
        ent->reservedC4 = 0;
        ent->routeMarkerIndex = 0;
        eventAddress.bytePointer = p;
        SeedCarLapProgress(ent, eventAddress.pointer->rivalStarts[0][0].modelId);
    }

    sub += g_RaceSeries * 144;
    base += sub;
    {
        u16 model;

        eventAddress.bytePointer = base;
        model = eventAddress.pointer->rivalStarts[0][0].modelId;
        ent->activeFlag = model;
        if ((s16)model != -1) {
            CarTrackLimitWork pair = {0};

            pair.limits.rightInset = 20;
            pair.limits.leftInset = -20;
            UpdateCarTrackState(ent, ent->trackPointIndex, &pair.limits);
            ent->modelY = ent->y;
            ent->previousTrackProgress = ent->trackProgress;
        }
    }

    {
        s32 height;

        height = ent->trackLateralOffset;
        ent->avoidanceStep = 0;
        ent->initialLateralOffset = height;
        ent->avoidanceTargetOffset = height;
        ent->aiLateralOffset = height;
    }
    CopyCarBodyRotationToModel(ent);
    {
        s32 lateral;

        lateral = ent->y;
        ent->reserved40 = 0;
        ent->steeringAngle = 0;
        ent->wheelRotation = 0;
        ent->modelY = lateral;
    }
}
