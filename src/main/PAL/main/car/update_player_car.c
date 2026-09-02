#include "game/state.h"
#include "game/race.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/track_internal.h"
#include "game/render.h"
#include "game/random.h"

#include "rage/trace.h"

/* Per-frame player physics orchestration and track contact. */
void UpdatePlayerCar(PlayerCarRuntime *car) {
    Matrix m1;
    Matrix m2;
    SVec sv1;
    Vec4 tmp;
    Matrix mA;
    SVec sv2;
    CarTrackLimits limits;
    GameCarDrive *p = &car->drive;
    s32 usesNegconMapping;
    s32 ground;
    s32 slip;
    s32 skid;
    s32 crash;
    s32 bodyY;
    u32 skidRange;

    TraceCarStates();

    usesNegconMapping = g_PadType == PAD_TYPE_NEGCON;
    car->facingBackwards = IsCarFacingBackwards(car);

    ShiftPlayerGears(car, usesNegconMapping);

    UpdateCarBodyRoll(car);

    if (car->verticalMotionState == 0) {
        UpdatePlayerSteeringTarget(car);
    }

    ReadPlayerCarInput(p);
    UpdateCarDrivetrain(car);

    UpdatePlayerControlFeedback(car);

    TraceCarMotion("pre-integrate", car);
    car->x -= car->motionX;
    car->z -= car->motionZ;
    BuildRotMatrixY(&m1, car->bodyYaw);
    BuildRotMatrixX(&m2, car->bodyPitch);
    MulMatrix2(&m2, &m1);
    BuildRotMatrixZ(&m2, car->bodyRoll);
    MulMatrix2(&m2, &m1);

    sv1.vx = 0;
    sv1.vy = 0;
    m2.m[0][0] = m1.m[0][0];
    m2.m[0][1] = m1.m[1][0];
    m2.m[0][2] = m1.m[2][0];
    m2.m[1][0] = m1.m[0][1];
    m2.m[1][1] = m1.m[1][1];
    m2.m[1][2] = m1.m[2][1];
    m2.m[2][0] = m1.m[0][2];
    m2.m[2][1] = m1.m[1][2];
    m2.m[2][2] = m1.m[2][2];
    sv1.vz = -p->bodyLiftOffset - 50;
    ApplyMatrix(&m2, &sv1, &car->motionX);

    /* Retail copied a stack Vec4 after assigning only X/Z. Preserve Y/W
     * explicitly so player state does not depend on the host stack ABI. */
    tmp = GetPlayerPosition(car);
    tmp.x = (p->accelPos * 6) / 1280 + car->x + car->motionX;
    tmp.z = (p->brakePos * 6) / 1280 + car->z + car->motionZ;
    SetPlayerPosition(car, &tmp);
    TraceCarMotion("post-position", car);
    AccumulateLapProgress(GetPlayerCarRuntime(car));
    TraceCarMotion("post-progress", car);

    slip = (car->bodyYaw - 0xC00 +
            TrackPoint(car->trackPointIndex)->angle) & 0xFFF;
    sv2.vx = 0;
    sv2.vz = 0;
    sv2.vy = slip;
    RotMatrix(&sv2, &mA);

    MeasurePlayerTrackLimits(&mA, &limits);

    if ((s16)car->motionTimer > 0) {
        ApplyCarKnockback(AsRivalCar(car));
    }
    TraceCarMotion("post-knockback", car);
    skid = UpdateCarTrackState(AsRivalCar(car), car->trackPointIndex, &limits);
    TraceCarMotion("post-track", car);
    skidRange = skid - 2;
    if (skidRange < 2U && car->speed < 64) {
        skid = 0;
    }

    if (p->shiftRpmDelta != 0) {
        s32 d = (g_CarSpec->revLimit + g_CarSpec->redline) / 2 - g_ShiftTargetRpm;
        if (d > 0) {
            car->bodyPitch += (d * Random15()) / 3276700;
        }
    }

    crash = CollidePlayerWithCars(car);
    TraceCarMotion(crash != 0 ? "post-cars-hit" : "post-cars-clear", car);
    if (skid != 0 || crash != 0) {
        StartCarBodyKick(2, AsRivalCar(car));
    }

    bodyY = car->y;
    CopyPlayerBodyRotationToModel(car);
    car->bodyRoll += car->bodyRollVelocity;
    car->modelY = car->y;
    /* Where the wheels sit, eight units under the body. */
    ground = bodyY - 8;

    UpdatePlayerJump(car, ground);

    UpdateCarTiltCounter(AsRivalCar(car));
    UpdateCarCrestHop(AsRivalCar(car));

    ApplyPlayerContactResponse(car, skid, crash);

    UpdatePlayerEnginePresentation(car);
}
