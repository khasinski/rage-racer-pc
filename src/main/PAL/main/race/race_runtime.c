#include "common.h"
#include "game/track_internal.h"
#include "game/prim.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/scratchpad.h"
#include "game/state.h"
#include "game/vector.h"
#include "game/waypoint.h"
#include "psyq/gpu.h"
#include "psyq/gte.h"

typedef struct CarTrackLimitWork {
    s32 reserved[4];
    CarTrackLimits limits;
} CarTrackLimitWork;


/*
 * Waypoint proximity test: returns 1 if the waypoint's (x,y) lies within a
 * +/-0x40 box around the player car centre, else 0.
 */

/*
 * Per-frame waypoint spawn/update state machine over the 6 slots. An idle slot
 * (active==0) that the car is near (IsCarNearWaypoint) spawns: increments the spawn
 * counter g_WaypointsCollected, plays cue 0xA, marks the slot active and seeds its
 * velocity from g_PlayerVelocity. An active slot integrates position from velocity
 * with 15/16 per-frame damping and grows its Z rotation toward 0x400, retiring to
 * state 2 once motion decays to zero. Register pins and raw tail-relative field
 * offsets are match-load-bearing.
 */

/* Counts how many of the 6 waypoint slots are active (active != 0). */


/*
 * The tail's multiply feeds the discarded rounding path below. GCC 2.6.3
 * removes that path and its mflo, but leaves the mult that sets the hard HI/LO
 * registers behind.
 */


/*
 * Initializes/spawns a route render object `ent`: reads a start entry from the
 * per-scene table (`arr` indexed by `pos`, g_TrackEventData base), sets the model id
 * (+0xAE / +0x122), start angle (0xC00 - track angle), zeroes the motion state
 * block, resolves the containing track point (FindTrackSegment) and builds the
 * initial marker geometry (UpdateCarTrackState). `ent` is a render/route object
 * accessed by raw byte offset (its first 0xE8 mirror GameRenderObject).
 */

s32 IsCarNearWaypoint(TrackWaypointRuntime *waypoint) {
    s32 center_x = g_PlayerCar.x;
    s32 x = waypoint->motion.x;
    s32 ret = 0;

    if ((center_x - 0x40) < x) {
        s32 max_x = center_x + 0x40;

        if (x < max_x) {
            s32 center_y = g_PlayerCar.z;
            s32 y = waypoint->motion.y;

            if ((center_y - 0x40) < y) {
                s32 max_y = center_y + 0x40;

                ret = y < max_y;
            }
        }
    }

    return ret;
}

void UpdateWaypoints(void) {
    TrackWaypointRuntime *waypoint;
    s32 i;
    s32 activeState;

    if (g_WaypointSpawnCooldown != 0) {
        g_WaypointSpawnCooldown--;
    }

    waypoint = g_Waypoints;
    i = 0;
    activeState = 1;
    do {
        if (waypoint->active == 0) {
            if (IsCarNearWaypoint(waypoint) != 0) {
                g_WaypointsCollected++;
                PlaySoundCue(0xA);

                waypoint->active = activeState;
                waypoint->motion.velocity.vector = g_PlayerVelocity[0];

                waypoint->motion.velocity.fields.x *= 2;
                waypoint->motion.velocity.fields.y *= 2;
                waypoint->motion.velocityMagnitude =
                    ((waypoint->motion.velocity.fields.x * waypoint->motion.velocity.fields.x) + (waypoint->motion.velocity.fields.y * waypoint->motion.velocity.fields.y)) /
                    0x2000;
            }
        } else if (waypoint->active == activeState) {
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

static inline void ClearScratchRenderMode37AAC(void) {
    g_ScratchRenderMode = 0;
}

/*
 * Renders the 6 waypoints. For each active-shaped slot it builds a rotation
 * matrix from the waypoint's Y and Z rotations and emits
 * two GTE draw primitives (SubmitModel) into the scratchpad OT: the second is
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
        MulMatrix2(SCRATCH_VIEW_MATRIX_GTE, &mtx0);
        BuildRotMatrixZ(mtx1Ptr, waypoint->motion.rotationZ);
        MulMatrix(&mtx0, mtx1Ptr);
        SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, &waypoint->motion, &mtx0);
        frameValue = g_ModelBankCount;
        ClearScratchRenderMode37AAC();
        drawArg = 1;
        if (drawId < frameValue) {
            drawArg = drawId;
        }
        SubmitModel(SCRATCHPAD, drawArg);

        BuildRotMatrixY(mtx1Ptr, 0x800);
        MulMatrix2(&mtx0, mtx1Ptr);
        SetGteObjectMatrix(SCRATCH_OBJECT_MATRIX_WORK, &waypoint->motion, mtx1Ptr);
        frameValue = g_ModelBankCount;
        ClearScratchRenderMode37AAC();
        drawArg = 1;
        if (drawId < frameValue) {
            drawArg = drawId;
        }
        SubmitModel(SCRATCHPAD, drawArg);

        i++;
        waypoint++;
    } while (i < 6);
}

s32 CountActiveWaypoints(void) {
    TrackWaypointRuntime *ptr = g_Waypoints;
    s32 count = 0;
    s32 i = 5;

    do {
        i--;
        count += ptr->active != 0;
        ptr++;
    } while (i >= 0);

    return count;
}

void DrawLapNumber(void) {
    SPRT *scratch;
    s32 track;
    s32 divisor;
    s32 digitsDrawn;
    s32 xOffset;
    s32 quotient;
    register SPRT *packet asm("$16");

    scratch = SCRATCH_PRIM_CURSOR_AS(SPRT);
    track = g_PlayerCar.lap;
    divisor = 1;
    digitsDrawn = 0;
    xOffset = 0;
    packet = scratch;

    while (1) {
        quotient = track / divisor;
        if (quotient == 0 && digitsDrawn > 0) {
            break;
        }

        {
            s32 y;
            SPRT *oldPacket;
            s32 tens;

            SetSprt(scratch);
            SetShadeTex(scratch, 1);

            y = 0x120 - xOffset;
            oldPacket = packet;
            tens = quotient / 10;
            divisor *= 10;
            xOffset += 0x18;
            digitsDrawn++;
            scratch++;
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
        void *ot;
        u8 *finalScratch;
        s32 tpage;
        RenderBufferAddress scratchAddress;

        scratchAddress.sprite = scratch;
        finalScratch = scratchAddress.bytes;
        packet = SCRATCHPAD_AS(SPRT);
        ot = GamePrimaryOrderingTable(0);
        tpage = 9;
        scratchAddress.sprite = packet;
        *scratchAddress.packetLink = finalScratch;
        *scratchAddress.packetLink = QueueDrawModePrim(ot, finalScratch, tpage);
    }
}

void DrawEndingScreen(void) {
    s16 *p;
    u32 sceneTimer;
    s32 x = 0;

    g_SceneTimer = g_SceneTimer + 1;
    {
        u32 openingFrame = g_SceneTimer;
        if (openingFrame < 61) {
            DrawFullscreenFadeTile(255 - (g_SceneTimer - 6) * 11, 0x49);
        }
    }
    {
        u32 endingFrame = g_SceneTimer;
        if (endingFrame >= 571 && g_EndingSceneLatch == 0) {
            g_EndingSceneLatch = 1;
        }
    }

    if (g_PlayerCar.progressB + g_PlayerCar.progressA >= g_PlayerCar.lap * g_TrackLength) {
        if (g_PlayerCar.lap < 257) {
            g_PlayerCar.lap = g_PlayerCar.lap + 1;
            SeedWaypoints();
        }
    }
    if (g_PlayerCar.lap >= 257) {
        if (g_RacePhase == 2) {
            g_RacePhase = 4;
            g_RaceFadeTimer = 0;
        }
    }

    if (g_RacePhase == 5) {
        if (g_RaceFadeTimer > 0) {
            DrawRaceEndBanner(g_RaceFadeTimer * 3);
            DrawFullscreenFadeTile(g_RaceFadeTimer * 3, 0x49);
            x = 6;
        }
        if (g_RaceFadeTimer >= 101) {
            ExitRaceScene(x);
        }
        g_RaceFadeTimer = g_RaceFadeTimer + 1;
    } else if (g_RacePhase == 4) {
        DrawText8x8(0x5c, 0x78, &g_TextCongratulations, 0x7811);
        DrawFullscreenFadeTile(g_RaceFadeTimer * 2, 0x29);
        g_RaceFadeTimer = g_RaceFadeTimer + 1;
        if (g_RaceFadeTimer < 201) {
            g_RaceFadeTimer = g_RaceFadeTimer + 1;
        } else {
            ExitRaceScene(6);
        }
    }

    sceneTimer = g_SceneTimer;
    g_AnimTimer = g_AnimTimer + 1;
    if (sceneTimer >= 90) {
        if (g_RacePhase == 0) {
            g_RacePhase = 1;
            goto race_intro_update_done;
        }
    } else {
        if (g_RacePhase == 0) {
            RunRaceIntroCamera(&g_PlayerCar, sceneTimer);
            g_EndingSceneLatch = 0;
            g_WaypointsCollected = 0;
            goto race_intro_update_done;
        }
    }
    if (g_RacePhase == 1) {
        u32 standingStartFrame = g_SceneTimer;
        if (standingStartFrame >= 211) {
            BeginCarStandingStart(&g_PlayerCar, sceneTimer);
            g_RacePhase = 2;
        }
    }
race_intro_update_done:

    if (g_RacePhase < 4) {
        DrawStartCountdown(g_SceneTimer);
        PlayCountdownCues(g_SceneTimer);
    }

    if (g_RacePhase > 0) {
        UpdatePlayerCar(&g_PlayerCar);
    } else if (g_RacePhase == 0) {
        UpdateLoadedAudioVoices(0, 1);
    }
    DrawLapNumber();

    if (g_RacePhase > 0) {
        UpdateCamera(CAMERA_VIEW_CAR, (GameRenderObject *)&g_PlayerCar);
    }

    p = &g_PlayerCar.trackSection;
    RequestTrackTexturePage(*p);
    UpdateEnvironment();
    DrawSkyBackground();
    SCRATCH_ENV_MODE4 = g_IsEnvironmentMode4;
    DrawTerrainCells();
    DrawCourseObjects();
    DrawCourseScenery(SeriesCourseIndex(), g_SceneTimer, 1);
    GetTrackZoneBlend(g_PlayerCar.trackProgress);
    SetReverbDepth(g_ReverbZoneDepth, g_ReverbZoneDepth);
    DrawPlayerTachometer();
    UpdateTrackEventSound(*p);
    if (g_RacePhase < 3) {
        UpdateWaypoints();
        DrawWaypoints();
    }
}

void ApplyTrackReverbZone(s32 position) {
    s32 result;
    s32 i;
    register s32 zone;
    s32 depth;
    register s32 scene;

    result = 0;
    if (position < 0) {
        position += g_TrackLength;
    }

    scene = g_RaceSeries;
    zone = 0;
    for (i = 0; i < 2; i++) {
        if (g_ReverbZones[scene][zone].start < position) {
            if (position < g_ReverbZones[scene][zone].end) {
                result = 0x46;
                break;
            }
        }
        zone++;
    }

    depth = result;
    SetReverbDepth(depth, depth);
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
            CarTrackLimitWork pair;

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

void InitRivalCarAi(GameCarRuntime *ent, s32 pos, RaceGridSlot *slots) {
  s32 pos2_R10;
  s32 idx_R8;
  register TrackEventDataAddress base_R9;
  register GameCarRuntime *ent2_R7;
  GameCarAiBlock *sub_R6;
  s32 c;
  u16 w;
  pos2_R10 = pos;
  idx_R8 = slots[pos2_R10].value;
  base_R9.pointer = g_TrackEventData;
  ent2_R7 = ent;
  if (!(idx_R8 < 12))
  {
    idx_R8 = 0;
  }
  {
    s32 lev1_R3;
    unsigned int idxoff1_R4;
    register TrackEventDataAddress p1_R4;
    lev1_R3 = g_RaceSeries;
    idxoff1_R4 = idx_R8;
    idxoff1_R4 = idxoff1_R4 * 16;
    p1_R4 = base_R9;
    p1_R4.bytePointer += idxoff1_R4 + lev1_R3 * 192;
    ent2_R7->targetSpeed =
        (p1_R4.pointer->rivalAiConfigs[0][0].speed * 1168) / 160;
    ent2_R7->accelerationStep =
        p1_R4.pointer->rivalAiConfigs[0][0].accelerationStep;
    ent2_R7->boostAccelerationThreshold =
        p1_R4.pointer->rivalAiConfigs[0][0].boostAccelerationThreshold;
    ent2_R7->collisionBoostDuration =
        p1_R4.pointer->rivalAiConfigs[0][0].collisionBoostDuration;
    ent2_R7->boostAcceleration =
        p1_R4.pointer->rivalAiConfigs[0][0].boostAcceleration;
  }
  __asm__ volatile("");
  c = ent2_R7->boostAccelerationThreshold;
  sub_R6 = GetCarAiBlock(ent2_R7);
  ent2_R7->boostTimer = 0;
  if (c < 0)
  {
    ent2_R7->boostAccelerationThreshold = 0;
  }
  else
    if (!(c < 11))
  {
    ent2_R7->boostAccelerationThreshold = 10;
  }
  if ((sub_R6->collisionBoostDuration) < 0)
  {
    sub_R6->collisionBoostDuration = 0;
  }
  c = sub_R6->boostAcceleration;
  if (c <= 0)
  {
    sub_R6->boostAcceleration = 0;
  }
  else
    if (!(c < 16))
  {
    sub_R6->boostAcceleration = 15;
  }
  {
    s32 lev2_R2;
    s32 idxoff2_R4;
    register TrackEventDataAddress p2_R3 asm("$3");
    lev2_R2 = g_RaceSeries;
    idxoff2_R4 = idx_R8 * 16;
    p2_R3 = base_R9;
    p2_R3.bytePointer += idxoff2_R4 + lev2_R2 * 192;
    w = p2_R3.pointer->rivalAiConfigs[0][0].minimumSpeed;
    sub_R6->minimumSpeed = w;
    if (((s16) w) < 0x3D)
    {
      sub_R6->minimumSpeed = 0x3C;
    }
    lev2_R2 = g_RaceSeries;
    __asm__("" : "=r"(idxoff2_R4) : "0"(idxoff2_R4));
    p2_R3 = base_R9;
    p2_R3.bytePointer += idxoff2_R4 + lev2_R2 * 192;
    w = p2_R3.pointer->rivalAiConfigs[0][0].initialEngineRpm;
    sub_R6->engineRpmLow = w;
    if (((s16) w) <= 0)
    {
      sub_R6->engineRpmLow = 0;
    }
  }
  {
    s32 v_R3;
    v_R3 = sub_R6->targetSpeed;
    sub_R6->accelerationLimit = (v_R3 * 6) / 100;
  }
  if (pos2_R10 >= 4)
  {
    s32 d_R5;
    d_R5 = g_TrackLength;
    sub_R6->gridTargetProgress = (d_R5 / 12) + ((d_R5 / 40) * (pos2_R10 - 4));
  }
  else
  {
    sub_R6->gridTargetProgress = g_TrackLength / 12;
  }
}
