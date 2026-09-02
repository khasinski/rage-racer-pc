#include "game/state.h"
#include "game/race.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/random.h"

#include "rage/trace.h"

/* Per-frame player physics orchestration and track contact. */
void UpdatePlayerCar(PlayerCarRuntime *car) {
    Vec4 tmp;
    GameCarDrive *p = &car->drive;
    s32 usesNegconMapping;
    s32 ground;
    s32 skid;
    s32 crash;
    s32 bodyY;

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
    CalculatePlayerBodyOffset(car);

    /* Retail copied a stack Vec4 after assigning only X/Z. Preserve Y/W
     * explicitly so player state does not depend on the host stack ABI. */
    tmp = GetPlayerPosition(car);
    tmp.x = (p->accelPos * 6) / 1280 + car->x + car->motionX;
    tmp.z = (p->brakePos * 6) / 1280 + car->z + car->motionZ;
    SetPlayerPosition(car, &tmp);
    TraceCarMotion("post-position", car);
    AccumulateLapProgress(AsRivalCar(car));
    TraceCarMotion("post-progress", car);

    skid = ResolvePlayerTrackContact(car);

    if (p->shiftRpmDelta != 0) {
        s32 d = (g_CarSpec->revLimit + g_CarSpec->redline) / 2 - g_ShiftTargetRpm;
        if (d > 0) {
            car->bodyPitch += (d * Random15()) / 3276700;
        }
    }

    crash = CollidePlayerWithCars(car);
    TraceCarMotion(crash != 0 ? "post-cars-hit" : "post-cars-clear", car);
    if (skid != 0 || crash != 0) {
        StartCarBodyKick(AsRivalCar(car), CAR_BODY_KICK_CORNERING);
    }

    bodyY = car->y;
    CopyPlayerBodyRotationToModel(car);
    car->bodyRoll += car->bodyRollVelocity;
    car->modelY = car->y;
    /* Where the wheels sit, eight units under the body. */
    ground = bodyY - 8;

    UpdatePlayerJump(car, ground);

    UpdatePlayerTilt(car);
    UpdateCarCrestHop(AsRivalCar(car));

    ApplyPlayerContactResponse(car, skid, crash);

    UpdatePlayerEnginePresentation(car);
}
