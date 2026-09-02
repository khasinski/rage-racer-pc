#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/state.h"

#include <stdio.h>
#include <string.h>

u8 g_PadType;
GameCarSpec *g_CarSpec;
s32 g_ShiftTargetRpm;

static GameCarSpec s_spec;
static char s_order[64];
static int s_orderLength;
static int s_traceCalls;
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

void TraceCarStates(void) { Step('A'); }

s32 IsCarFacingBackwards(const PlayerCarRuntime *car) {
    (void)car;
    Step('B');
    return 1;
}

void ShiftPlayerGears(PlayerCarRuntime *car, int mapping) {
    (void)car;
    s_shiftMapping = mapping;
    Step('C');
}

void UpdateCarBodyRoll(PlayerCarRuntime *car) {
    (void)car;
    Step('D');
}

void UpdatePlayerSteeringTarget(PlayerCarRuntime *car) {
    (void)car;
    Step('E');
}

void ReadPlayerCarInput(GameCarDrive *drive) {
    (void)drive;
    Step('F');
}

void UpdateCarDrivetrain(PlayerCarRuntime *car) {
    (void)car;
    Step('G');
}

void UpdatePlayerControlFeedback(PlayerCarRuntime *car) {
    (void)car;
    Step('H');
}

void CalculatePlayerBodyOffset(PlayerCarRuntime *car) {
    car->motionX = 5;
    car->motionY = 0;
    car->motionZ = 7;
    Step('I');
}

void AccumulateLapProgress(GameCarRuntime *car) {
    (void)car;
    Step('J');
}

s32 ResolvePlayerTrackContact(PlayerCarRuntime *car) {
    (void)car;
    Step('K');
    return s_skid;
}

s32 Random15(void) {
    Step('L');
    return 32767;
}

s32 CollidePlayerWithCars(PlayerCarRuntime *car) {
    (void)car;
    Step('M');
    return s_crash;
}

void StartCarBodyKick(s32 strength, GameCarRuntime *car) {
    (void)car;
    if (strength != 2) {
        s_failures++;
    }
    Step('N');
}

void UpdatePlayerJump(PlayerCarRuntime *car, s32 groundHeight) {
    (void)car;
    s_jumpGround = groundHeight;
    Step('P');
}

void UpdateCarTiltCounter(PlayerCarRuntime *car) {
    (void)car;
    Step('Q');
}

void UpdateCarCrestHop(GameCarRuntime *car) {
    (void)car;
    Step('R');
}

void ApplyPlayerContactResponse(PlayerCarRuntime *car, s32 skid, s32 crash) {
    (void)car;
    s_responseSkid = skid;
    s_responseCrash = crash;
    Step('S');
}

void UpdatePlayerEnginePresentation(PlayerCarRuntime *car) {
    (void)car;
    Step('T');
}

void TraceCarMotion(const char *phase, PlayerCarRuntime *car) {
    (void)phase;
    (void)car;
    s_traceCalls++;
}

static void Reset(PlayerCarRuntime *car) {
    memset(car, 0, sizeof(*car));
    memset(&s_spec, 0, sizeof(s_spec));
    s_spec.revLimit = 8000;
    s_spec.redline = 6000;
    g_CarSpec = &s_spec;
    g_ShiftTargetRpm = 7000;
    g_PadType = PAD_TYPE_DIGITAL;
    s_orderLength = 0;
    s_order[0] = '\0';
    s_traceCalls = 0;
    s_skid = 0;
    s_crash = 0;
    s_jumpGround = 0;
    s_responseSkid = 0;
    s_responseCrash = 0;
    s_shiftMapping = -1;
    car->x = 100;
    car->y = 50;
    car->z = 200;
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
    UpdatePlayerCar(&car);
    CheckOrder("ABCDEFGHIJKMPQRST");
    CHECK(car.facingBackwards == 1 && s_shiftMapping == 0);
    CHECK(car.x == 95 && car.z == 187);
    CHECK(s_jumpGround == 42);
    CHECK(s_responseSkid == 0 && s_responseCrash == 0);
    CHECK(s_traceCalls == 4);

    Reset(&car);
    g_PadType = PAD_TYPE_NEGCON;
    car.verticalMotionState = 1;
    car.drive.shiftRpmDelta = 1;
    g_ShiftTargetRpm = 0;
    s_skid = 3;
    s_crash = 1;
    UpdatePlayerCar(&car);
    CheckOrder("ABCDFGHIJKLMNPQRST");
    CHECK(s_shiftMapping == 1);
    CHECK(car.bodyPitch > 0);
    CHECK(s_responseSkid == 3 && s_responseCrash == 1);

    if (s_failures != 0) {
        printf("%d player update orchestration checks failed\n", s_failures);
        return 1;
    }
    puts("player update orchestrates each physics stage in order");
    return 0;
}
