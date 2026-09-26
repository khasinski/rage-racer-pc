#include "game/car.h"
#include "game/car_control.h"
#include "game/race_sim.h"
#include "game/car_internal.h"
#include "game/input_internal.h"
#include "game/race.h"
#include "game/state.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

u8 g_PadType;
u16 g_PadHeld;
u16 g_PadPressed;
GameCarSpec *g_CarSpec;
u16 g_PadButtonMapping[16];
ControllerMappingIndex g_NegconMappingIndex;
s16 g_NegconAnalogI;
s16 g_NegconAnalogII;
s16 g_NegconAnalogL;
s16 g_RacePhase;
s16 g_PlayerAutoSteer;
s16 g_NegconSteer;
NegconCalibrationValue g_NegconMaxTwist;
const s16 g_NegconSteerRange[NEGCON_STEER_RANGE_COUNT] = {25, 38, 75, 113};

static int s_failures;

static void Check(GameCarDrive *drive, s32 accelerator, s32 brake,
                  const char *description) {
    const DriverInput input = ReadDriverInput();
    if (input.throttle != accelerator || input.brake != brake) {
        printf("FAIL snapshot %s\n", description);
        s_failures++;
    }
    if (drive->acceleratorInput.value != accelerator ||
        drive->brakeInput != brake) {
        printf("FAIL %s: accelerator=%d brake=%d; expected %d,%d\n",
               description, drive->acceleratorInput.value, drive->brakeInput,
               accelerator, brake);
        s_failures++;
    }
}

static void CheckNegconMapping(s16 mapping, s32 accelerator, s32 brake) {
    GameCarDrive drive;
    char description[40];

    memset(&drive, 0, sizeof(drive));
    g_NegconMappingIndex = mapping;
    ReadPlayerCarInput(&drive);
    snprintf(description, sizeof(description), "NeGcon mapping %d", mapping);
    Check(&drive, accelerator, brake, description);
}

int main(void) {
    GameCarDrive drive;
    GameCarSpec spec = {0};
    PlayerCarRuntime car = {0};

    spec.topGear = 6;
    g_CarSpec = &spec;
    car.drive.manual = 1;
    g_PadButtonMapping[4] = 0x08;
    g_PadButtonMapping[5] = 0x04;
    g_PadButtonMapping[12] = 0x100;
    g_PadButtonMapping[13] = 0x200;
    const u16 buttons[] = {0x08, 0x04, 0x100, 0x200, 0x08};
    const s16 gears[] = {4, 2, 4, 2, 3};
    for (int i = 0; i < 5; i++) {
        car.drive.gear = 3;
        g_PadPressed = buttons[i];
        ShiftPlayerGears(&car, i >= 2);
        if (car.drive.gear != gears[i]) {
            printf("FAIL shift mapping %d: gear=%d\n", i, car.drive.gear);
            s_failures++;
        }
    }

    car.steeringAngle = 0;
    car.drive.steerHoldFrames = 20;
    UpdateCarControlFeedback(&car, 1);
    if (car.drive.steerHoldFrames != -10) s_failures++;
    car.drive.steerHoldFrames = 20;
    UpdateCarControlFeedback(&car, 0);
    if (car.drive.steerHoldFrames != 0) s_failures++;

    memset(&car, 0, sizeof(car));
    car.speed = 800;
    g_RacePhase = RACE_PHASE_ACTIVE;
    g_PadType = PAD_TYPE_DIGITAL;
    g_PadButtonMapping[0] = 1;
    g_PadButtonMapping[1] = 2;
    g_PadHeld = 3;
    UpdateCarBodyRoll(&car);
    if (car.drive.steerPos != -1536 || car.drive.trackCurveMode != 2) {
        puts("FAIL digital steering mapping or left-button priority");
        s_failures++;
    }
    memset(&car, 0, sizeof(car));
    car.speed = 800;
    g_PadType = PAD_TYPE_NEGCON;
    g_NegconSteer = 64;
    g_NegconMaxTwist = -1;
    UpdateCarBodyRoll(&car);
    if (car.drive.trackCurveMode != 1 || car.drive.steerPos != 1024) {
        puts("FAIL analog steering with invalid calibration");
        s_failures++;
    }
    g_RacePhase = RACE_PHASE_COUNTDOWN;
    UpdateCarBodyRoll(&car);
    if (car.drive.steerPos != 0 || car.bodyRollVelocity != 0 ||
        car.steeringAngle != 0) {
        puts("FAIL countdown should center steering");
        s_failures++;
    }
    g_RacePhase = RACE_PHASE_FINISHED;
    car.trackLateralOffset = 0;
    car.trackHeading = 0xC00;
    car.bodyYaw = 0;
    g_PadHeld = 3;
    UpdateCarBodyRoll(&car);
    if (car.drive.steerPos != 0 || car.bodyRollVelocity != 0) {
        puts("FAIL finished race should use automatic steering");
        s_failures++;
    }

    memset(g_PadButtonMapping, 0, sizeof(g_PadButtonMapping));
    g_PadButtonMapping[2] = 0x01;
    g_PadButtonMapping[3] = 0x02;
    g_PadButtonMapping[10] = 0x04;
    g_PadButtonMapping[11] = 0x08;
    g_RacePhase = 2;

    memset(&drive, 0, sizeof(drive));
    g_PadType = PAD_TYPE_DIGITAL;
    g_PadHeld = 0x01;
    ReadPlayerCarInput(&drive);
    Check(&drive, 0x100, 0, "digital accelerator");
    g_PadHeld = 0x02;
    ReadPlayerCarInput(&drive);
    Check(&drive, 0, 0x100, "digital brake");

    g_PadType = PAD_TYPE_NEGCON;
    g_PadHeld = 0x0C;
    g_NegconAnalogI = 53;
    g_NegconAnalogII = 106;
    g_NegconAnalogL = 26;
    CheckNegconMapping(0, 128, 256);
    CheckNegconMapping(1, 256, 128);
    CheckNegconMapping(2, 256, (26 << 8) / 106);
    CheckNegconMapping(3, 256, (26 << 8) / 106);
    CheckNegconMapping(4, 256, 256);
    CheckNegconMapping(5, 128, 256);
    CheckNegconMapping(6, 256, 128);
    CheckNegconMapping(7, 256, 256);
    CheckNegconMapping(INT16_MIN, 256, 256);
    CheckNegconMapping(INT16_MAX, 256, 256);

    g_NegconAnalogI = 0;
    g_NegconAnalogII = 106;
    CheckNegconMapping(0, 0, 256);

    g_NegconAnalogI = INT16_MAX;
    g_NegconAnalogII = INT16_MIN;
    CheckNegconMapping(0, 13599, -13601);

    memset(&drive, 0x7F, sizeof(drive));
    g_PadType = 0;
    ReadPlayerCarInput(&drive);
    Check(&drive, 0, 0, "unsupported controller");

    memset(&drive, 0x7F, sizeof(drive));
    g_PadType = PAD_TYPE_DIGITAL;
    g_PadHeld = 0x03;
    g_RacePhase = 4;
    ReadPlayerCarInput(&drive);
    Check(&drive, 0, 0, "finished race");

    g_RacePhase = RACE_PHASE_ACTIVE;
    g_PlayerAutoSteer = 0;
    g_PadButtonMapping[4] = 0x08;
    g_PadButtonMapping[5] = 0x04;
    g_PadButtonMapping[12] = 0x100;
    g_PadButtonMapping[13] = 0x200;
    g_PadPressed = 0x08;
    DriverInput sampled = ReadDriverInput();
    if (!sampled.shiftUp || sampled.shiftDown ||
        sampled.steering.mode != STEERING_DIGITAL) s_failures++;
    g_PadType = PAD_TYPE_NEGCON;
    g_PadPressed = 0x200;
    sampled = ReadDriverInput();
    if (sampled.shiftUp || !sampled.shiftDown ||
        sampled.steering.mode != STEERING_ANALOG) s_failures++;
    g_RacePhase = RACE_PHASE_FINISHED;
    sampled = ReadDriverInput();
    if (sampled.throttle != 0 || sampled.brake != 0 ||
        sampled.steering.mode != STEERING_AUTOMATIC) s_failures++;

    /* A separate race samples devices independently of the legacy scene. */
    g_PadButtonMapping[0] = 1; g_PadButtonMapping[1] = 2;
    g_PadType = PAD_TYPE_DIGITAL;
    g_PadHeld = g_PadButtonMapping[0] | g_PadButtonMapping[2];
    g_PadPressed = g_PadButtonMapping[4];
    g_PlayerAutoSteer = 1;
    DriverInput controls = ReadCarControls();
    if (controls.steering.mode != STEERING_DIGITAL || !controls.steering.left ||
        controls.throttle != 256 || !controls.shiftUp) s_failures++;
    g_RacePhase = RACE_PHASE_ACTIVE;
    DriverInput same = ReadCarControls();
    if (controls.steering.mode != same.steering.mode ||
        controls.steering.left != same.steering.left ||
        controls.steering.right != same.steering.right ||
        controls.steering.angle != same.steering.angle ||
        controls.throttle != same.throttle || controls.brake != same.brake ||
        controls.shiftUp != same.shiftUp || controls.shiftDown != same.shiftDown) s_failures++;
    RaceSim race = {.phase = SIM_RACING};
    race.drivers[11].status = SIM_DRIVING;
    if (!SetRaceInput(&race, 11, &controls) || race.drivers[11].input.throttle != 256) s_failures++;
    g_PadType = PAD_TYPE_NEGCON;
    g_NegconMappingIndex = 0;
    g_NegconAnalogI = INT16_MAX; g_NegconAnalogII = INT16_MIN;
    g_NegconSteer = INT16_MAX;
    controls = ReadCarControls();
    if (controls.throttle < 0 || controls.throttle > 256 ||
        controls.brake < 0 || controls.brake > 256 ||
        controls.steering.angle < -13 * 512 || controls.steering.angle > 13 * 512 ||
        !SetRaceInput(&race, 11, &controls)) s_failures++;

    const s16 pressures[] = {INT16_MIN, -1, 0, 53, 106, 107, 20000, INT16_MAX};
    const s16 expected[] = {0, 0, 0, 128, 256, 256, 256, 256};
    for (size_t i = 0; i < sizeof(pressures) / sizeof(pressures[0]); ++i) {
        g_NegconAnalogI = pressures[i];
        controls = ReadCarControls();
        if (controls.throttle != expected[i]) {
            printf("FAIL bounded pressure %d: %d instead of %d\n",
                   pressures[i], controls.throttle, expected[i]);
            s_failures++;
        }
    }

    if (s_failures != 0) {
        printf("%d player input checks failed\n", s_failures);
        return 1;
    }
    puts("player pedals follow every controller mapping");
    return 0;
}
