#include "game/car_control.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/state.h"
#include "game/track_internal.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

u32 g_RandomSeed;
u8 g_PadType;
GameCarSpec *g_CarSpec;
s16 g_RacePhase;
s32 g_RaceSeries;
const GameTrackPoint *g_TrackPoints;
const GameTrackArcCenter *g_TrackArcCenters;
s32 g_TrackPointCount;
s32 g_TrackLength;
const TrackEventData *g_TrackEventData;

static GameCarSpec s_spec;
static char s_order[64];
static int s_orderLength;
static s32 s_skid;
static s32 s_crash;
static s32 s_jumpGround;
static s32 s_responseSkid;
static s32 s_responseCrash;
static int s_shiftMapping;
static int s_failures;

static void Step(char step) {
    s_order[s_orderLength++] = step;
    s_order[s_orderLength] = '\0';
}

s32 CarFacesBackwards(const PlayerCarRuntime *car, const TrackRoute *route) {
    (void)car;
    if (route->points != g_TrackPoints || route->count != g_TrackPointCount) s_failures++;
    Step('B');
    return 1;
}

DriverInput ReadDriverInput(void) {
    return (DriverInput){.steering.mode =
        g_PadType == PAD_TYPE_NEGCON ? STEERING_ANALOG : STEERING_DIGITAL};
}

void ApplyDriverInput(PlayerCarRuntime *car, const GameCarSpec *spec,
                       const DriverInput *input) {
    (void)spec;
    s_shiftMapping = input->steering.mode == STEERING_ANALOG;
    Step('C');
    Step('D');
    if (car->verticalMotionState == CAR_VERTICAL_GROUNDED) Step('E');
    Step('F');
}

void UpdateCarDrivetrain(PlayerCarRuntime *car) {
    (void)car;
    Step('G');
}

void UpdateCarControlFeedback(PlayerCarRuntime *car, int analog) {
    (void)car;
    if (analog != (g_PadType == PAD_TYPE_NEGCON)) s_failures++;
    Step('H');
}

void CalculatePlayerBodyOffset(PlayerCarRuntime *car) {
    car->motionX = 5;
    car->motionY = 0;
    car->motionZ = 7;
    Step('I');
}

s32 FindCarTrackSegment(const GameCarRuntime *car, const TrackRoute *route, s32 start) {
    (void)car; (void)route;
    return start;
}

void MoveCarTrackProgress(GameCarRuntime *car, const TrackRoute *route, s32 target, int reverse) {
    (void)car;
    (void)route; (void)target; (void)reverse;
    Step('J');
}

s32 ResolveCarTrackContact(PlayerCarRuntime *car, const TrackRoute *route,
                          const CarHullPoint *corners, int reverse) {
    (void)car;
    (void)route; (void)corners; (void)reverse;
    Step('K');
    return s_skid;
}

s32 CollidePlayerWithCars(PlayerCarRuntime *car) {
    (void)car;
    Step('M');
    return s_crash;
}

void BeginCarBodyKick(GameCarRuntime *car, CarBodyKickMode mode, s32 heading, s32 random) {
    (void)car;
    (void)heading;
    (void)random;
    if (mode != CAR_BODY_KICK_CORNERING) {
        s_failures++;
    }
    Step('N');
}

int StepPlayerJump(PlayerCarRuntime *car, const GameCarSpec *spec, s32 groundHeight) {
    (void)car;
    (void)spec;
    s_jumpGround = groundHeight;
    Step('P');
    return 0;
}

void UpdateCarTilt(PlayerCarRuntime *car, const GameCarSpec *spec, int racing) {
    (void)car;
    (void)spec;
    (void)racing;
    Step('Q');
}

void StepCarCrestHop(GameCarRuntime *car, const TrackEventData *events, s32 length, int reverse) {
    (void)car;
    (void)events;
    (void)length;
    (void)reverse;
    Step('R');
}

s32 ApplyCarContactResponse(PlayerCarRuntime *car, const GameTrackPoint *point, s32 skid, s32 crash) {
    (void)car;
    (void)point;
    s_responseSkid = skid;
    s_responseCrash = crash;
    Step('S');
    return 512;
}

void PlayPlayerLandingCue(s32 frames, int audible) {
    if (audible != (g_RacePhase <= RACE_PHASE_ACTIVE)) s_failures++;
    if (frames != 0) s_failures++;
    Step('U');
}
void PlayPlayerContactCue(const PlayerCarRuntime *car, s32 skid, s32 slip, int audible) {
    if (audible != (g_RacePhase <= RACE_PHASE_ACTIVE)) s_failures++;
    (void)car;
    if (skid != s_responseSkid || slip != 512) s_failures++;
    Step('V');
}

void UpdatePlayerEnginePresentation(const PlayerCarRuntime *car, const GameCarSpec *spec, int finished) {
    if (finished != (g_RacePhase >= RACE_PHASE_FINISHED)) s_failures++;
    if (spec != g_CarSpec) s_failures++;
    (void)car;
    Step('T');
}

static void Reset(PlayerCarRuntime *car) {
    memset(car, 0, sizeof(*car));
    memset(&s_spec, 0, sizeof(s_spec));
    s_spec.revLimit = 8000;
    s_spec.redline = 6000;
    g_CarSpec = &s_spec;
    g_RandomSeed = 24884;
    car->drive.shiftTargetRpm = 7000;
    g_PadType = PAD_TYPE_DIGITAL;
    s_orderLength = 0;
    s_order[0] = '\0';
    s_skid = 0;
    s_crash = 0;
    s_jumpGround = 0;
    s_responseSkid = 0;
    s_responseCrash = 0;
    s_shiftMapping = -1;
    car->x = 100;
    car->y = 50;
    car->z = 200;
    car->positionW = 77;
    car->motionX = 10;
    car->motionZ = 20;
}

static void CheckOrder(const char *expected) {
    if (strcmp(s_order, expected) != 0) {
        printf("FAIL order: got %s expected %s\n", s_order, expected);
        s_failures++;
    }
}

#define CHECK(condition) do {                                                \
    if (!(condition)) {                                                      \
        printf("FAIL line %d: %s\n", __LINE__, #condition);                 \
        s_failures++;                                                        \
    }                                                                        \
} while (0)

int main(void) {
    PlayerCarRuntime car;

    Reset(&car);
    car.drive.accelPos = 640;
    car.drive.brakePos = 1280;
    UpdatePlayerCar(&car);
    CheckOrder("BCDEFGHIJKMPQRSUVT");
    CHECK(car.facingBackwards == 1 && s_shiftMapping == 0);
    CHECK(car.x == 98 && car.z == 193);
    CHECK(car.y == 50 && car.positionW == 77);
    CHECK(s_jumpGround == 42);
    CHECK(s_responseSkid == 0 && s_responseCrash == 0);

    Reset(&car);
    g_PadType = PAD_TYPE_NEGCON;
    car.verticalMotionState = 1;
    car.drive.shiftRpmDelta = 1;
    car.drive.shiftTargetRpm = 0;
    s_skid = 3;
    s_crash = 1;
    UpdatePlayerCar(&car);
    CheckOrder("BCDFGHIJKMNPQRSUVT");
    CHECK(s_shiftMapping == 1);
    CHECK(car.bodyPitch > 0);
    CHECK(s_responseSkid == 3 && s_responseCrash == 1);

    Reset(&car);
    car.x = INT_MIN;
    car.z = INT_MAX;
    car.y = INT_MIN;
    car.motionX = INT_MAX;
    car.motionZ = INT_MIN;
    car.drive.accelPos = INT_MAX;
    car.drive.brakePos = INT_MIN;
    car.drive.shiftRpmDelta = 1;
    car.bodyPitch = INT_MAX;
    car.bodyRoll = INT_MAX;
    car.bodyRollVelocity = INT_MAX;
    car.drive.shiftTargetRpm = -100000;
    UpdatePlayerCar(&car);
    CHECK(car.x == 6 && car.z == 6);
    CHECK(car.bodyPitch == 2147483407);
    CHECK(car.bodyRoll == -2);
    CHECK(s_jumpGround == 2147483640);

    if (s_failures != 0) {
        printf("%d player update orchestration checks failed\n", s_failures);
        return 1;
    }
    puts("player update orchestrates each physics stage in order");
    return 0;
}
