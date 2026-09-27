#include "game/race_sim.h"
#include <string.h>

int SaveRaceFrame(const RaceSim *race, RaceFrame *frame) {
    if (!race || !frame) return 0;
    RaceFrame saved = {.track = race->route.points, .events = race->events,
        .laps = race->laps, .reverse = race->reverse, .finishCount = race->finishCount,
        .tick = race->tick, .countdown = race->countdown, .elapsed = race->elapsed, .phase = race->phase};
    for (int seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) {
        const SimDriver *driver = &race->drivers[seat];
        DriverFrame *copy = &saved.drivers[seat];
        *copy = (DriverFrame){.car = driver->car, .input = driver->input, .step = driver->step,
            .stepTick = driver->stepTick, .inputTick = driver->inputTick, .random = driver->random, .lapStarted = driver->lapStarted,
            .finishTick = driver->finishTick, .crashed = driver->crashed, .rival = driver->rival,
            .variant = driver->variant, .rivalSlot = driver->rivalSlot, .place = driver->place,
            .wrongWayFrames = driver->wrongWayFrames, .status = driver->status};
        memcpy(copy->lapTicks, driver->lapTicks, sizeof(copy->lapTicks));
    }
    if (!ValidRaceFrame(race, &saved)) return 0;
    *frame = saved;
    return 1;
}

int ValidRaceFrame(const RaceSim *race, const RaceFrame *frame) {
    if (!race || !frame || frame->track != race->route.points || frame->events != race->events ||
        frame->laps != race->laps || frame->reverse != race->reverse ||
        frame->laps < 1 || frame->laps > PLAYER_LAP_TIME_CAPACITY ||
        (u32)frame->reverse > 1 || !race->route.points || race->route.count <= 0 || race->route.length <= 0 ||
        (u32)frame->phase > SIM_FINISHED || frame->elapsed > frame->tick ||
        frame->finishCount < 0 || frame->finishCount > DRIVER_SEAT_LIMIT ||
        ((frame->phase == SIM_SETUP || frame->phase == SIM_COUNTDOWN) && frame->elapsed) ||
        (frame->phase == SIM_SETUP && frame->countdown) ||
        (frame->phase == SIM_COUNTDOWN && !frame->countdown) ||
        (frame->phase == SIM_RACING && frame->countdown) ||
        (frame->phase == SIM_FINISHED && frame->countdown && (frame->elapsed || frame->finishCount))) return 0;
    unsigned places = 0;
    int finished = 0;
    for (int seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) {
        const SimDriver *driver = &race->drivers[seat];
        const DriverFrame *copy = &frame->drivers[seat];
        if (copy->variant != driver->variant || copy->rival != driver->rival ||
            copy->rivalSlot != driver->rivalSlot || (u32)copy->status > SIM_RETIRED ||
            (copy->status == SIM_EMPTY) != (driver->status == SIM_EMPTY) ||
            (copy->status != SIM_EMPTY && (u32)copy->car.trackPointIndex >= (u32)race->route.count) ||
            copy->stepTick > frame->tick || copy->inputTick > frame->tick || copy->lapStarted > frame->elapsed ||
            copy->finishTick > frame->elapsed || !ValidDriverInput(&copy->input)) return 0;
        if (copy->wrongWayFrames < 0 || (u32)copy->crashed > 1 ||
            (u32)copy->step.motionFinished > 1 || copy->step.landingFrames < 0) return 0;
        for (int lap = 0; lap < PLAYER_LAP_TIME_CAPACITY; ++lap)
            if (copy->lapTicks[lap] > frame->elapsed) return 0;
        if (copy->status == SIM_DRIVER_FINISHED) {
            if (copy->place < 1 || copy->place > frame->finishCount ||
                (places & (1u << copy->place))) return 0;
            places |= 1u << copy->place;
            finished++;
        } else if (copy->place != 0) return 0;
        if (frame->phase == SIM_FINISHED && copy->status == SIM_DRIVING) return 0;
        if (copy->status == SIM_EMPTY) continue;
        const PlayerCarRuntime *car = &copy->car;
        if (car->modelIndex != driver->car.modelIndex ||
            (u32)car->facingBackwards > 1 ||
            (u32)car->verticalMotionState > CAR_VERTICAL_FALLING ||
            car->lap < 0 || car->lap > frame->laps + 1 ||
            (copy->status == SIM_DRIVING && (car->activeFlag == -1 ||
                (u32)car->previousTrackPointIndex >= (u32)race->route.count)) ||
            (copy->status != SIM_DRIVING && car->activeFlag != -1)) return 0;
        if (copy->rival) {
            const GameCarRuntime *rival = AsConstRivalCar(car);
            if (rival->acceleratorInput < 0 || rival->acceleratorInput > 256 ||
                rival->brakeInput < 0 || rival->brakeInput > 256) return 0;
        } else {
            const GameCarDrive *drive = &car->drive;
            if (drive->manual != driver->car.drive.manual ||
                drive->gear < CAR_FIRST_FORWARD_GEAR || drive->gear > CAR_FORWARD_GEAR_COUNT ||
                (u32)drive->motionState > CAR_MOTION_STANDING_START ||
                drive->acceleratorInput.value < 0 || drive->acceleratorInput.value > 256 ||
                drive->brakeInput < 0 || drive->brakeInput > 256 ||
                (u32)drive->acceleratorLatch > 2 || (u32)drive->brakeLatch > 2) return 0;
        }
    }
    return finished == frame->finishCount;
}

int RestoreRaceFrame(RaceSim *race, const RaceFrame *frame) {
    if (!ValidRaceFrame(race, frame)) return 0;
    race->phase = frame->phase; race->tick = frame->tick; race->countdown = frame->countdown;
    race->elapsed = frame->elapsed; race->finishCount = frame->finishCount;
    for (int seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) {
        SimDriver *driver = &race->drivers[seat];
        const DriverFrame *copy = &frame->drivers[seat];
        driver->car = copy->car; driver->input = copy->input; driver->step = copy->step;
        driver->stepTick = copy->stepTick; driver->inputTick = copy->inputTick; driver->random = copy->random;
        driver->lapStarted = copy->lapStarted; driver->finishTick = copy->finishTick;
        driver->crashed = copy->crashed; driver->place = copy->place;
        driver->wrongWayFrames = copy->wrongWayFrames; driver->status = copy->status;
        memcpy(driver->lapTicks, copy->lapTicks, sizeof(driver->lapTicks));
    }
    return 1;
}


/* Checkpoint format 1: named scalar fields, never native struct bytes.
 * Offsets locate fields in this build only; the table order/width is the wire
 * schema. Explicit reserved fields are retained for exact state restoration.
 * Anonymous unions are encoded once, using the seat's human/AI interpretation.
 * Changing these lists requires a checkpoint format version change. */
typedef struct FrameField { size_t offset; unsigned width, count; } FrameField;
#define FRAME_FIELD(type, member, width, count) {offsetof(type, member), width, count},
#define FRAME_WIDTH(type, member, width, count) \
    _Static_assert(sizeof(((type *)0)->member) == width * count, "checkpoint field width changed");
#define FRAME_BYTES(type, member, width, count) + width * count

#define FRAME_FIELDS(X) \
    X(RaceFrame, laps, 4, 1) X(RaceFrame, reverse, 4, 1) X(RaceFrame, finishCount, 4, 1) \
    X(RaceFrame, tick, 4, 1) X(RaceFrame, countdown, 4, 1) X(RaceFrame, elapsed, 4, 1) \
    X(RaceFrame, phase, 4, 1)
FRAME_FIELDS(FRAME_WIDTH)
static const FrameField frame_fields[] = { FRAME_FIELDS(FRAME_FIELD) };
enum { FRAME_FIELDS_BYTES = 0 FRAME_FIELDS(FRAME_BYTES) };

#define DRIVER_FIELDS(X) \
    X(DriverFrame, stepTick, 4, 1) X(DriverFrame, inputTick, 4, 1) X(DriverFrame, random, 4, 1) \
    X(DriverFrame, lapStarted, 4, 1) X(DriverFrame, finishTick, 4, 1) X(DriverFrame, lapTicks, 4, 6) \
    X(DriverFrame, crashed, 4, 1) X(DriverFrame, rival, 4, 1) X(DriverFrame, variant, 4, 1) \
    X(DriverFrame, rivalSlot, 4, 1) X(DriverFrame, place, 4, 1) X(DriverFrame, wrongWayFrames, 4, 1) \
    X(DriverFrame, status, 4, 1) X(DriverFrame, step.skid, 4, 1) X(DriverFrame, step.motionFinished, 4, 1) \
    X(DriverFrame, step.landingFrames, 4, 1) X(DriverFrame, step.skidAngle, 4, 1) X(DriverFrame, input.steering.mode, 4, 1) \
    X(DriverFrame, input.steering.left, 4, 1) X(DriverFrame, input.steering.right, 4, 1) X(DriverFrame, input.steering.angle, 4, 1) \
    X(DriverFrame, input.throttle, 2, 1) X(DriverFrame, input.brake, 2, 1) X(DriverFrame, input.shiftUp, 4, 1) \
    X(DriverFrame, input.shiftDown, 4, 1)
DRIVER_FIELDS(FRAME_WIDTH)
static const FrameField driver_fields[] = { DRIVER_FIELDS(FRAME_FIELD) };
enum { DRIVER_FIELDS_BYTES = 0 DRIVER_FIELDS(FRAME_BYTES) };

#define COMMON_CAR_FIELDS(X, type) \
    X(type, x, 4, 1) X(type, y, 4, 1) X(type, z, 4, 1) \
    X(type, positionW, 4, 1) X(type, motionX, 4, 1) X(type, motionY, 4, 1) \
    X(type, motionZ, 4, 1) X(type, reserved1C, 4, 1) X(type, bodyPitch, 4, 1) \
    X(type, bodyYaw, 4, 1) X(type, bodyRoll, 4, 1) X(type, bodyRotationW, 4, 1) \
    X(type, trackPointIndex, 4, 1) X(type, trackLateralOffset, 4, 1) X(type, segmentFraction, 4, 1) \
    X(type, normalizedLateralOffset, 4, 1) X(type, reserved40, 4, 1) X(type, steeringAngle, 4, 1) \
    X(type, wheelRotation, 4, 1) X(type, reserved4C, 4, 1) X(type, modelPitch, 4, 1) \
    X(type, modelYaw, 4, 1) X(type, modelRoll, 4, 1) X(type, modelRotationW, 4, 1) \
    X(type, modelY, 4, 1) X(type, bodyRollVelocity, 4, 1) X(type, progressA, 4, 1) \
    X(type, progressB, 4, 1) X(type, trackProgress, 4, 1) X(type, previousTrackProgress, 4, 1) \
    X(type, trackSection, 2, 1) X(type, reserved7A, 2, 1) X(type, velocityX, 2, 1) \
    X(type, velocityZ, 2, 1) X(type, motionActive, 2, 1) X(type, motionTimer, 2, 1) \
    X(type, motionMode, 2, 1) X(type, motionModeTimer, 2, 1) X(type, motionValue, 2, 1) \
    X(type, collisionFlag, 2, 1) X(type, tiltCounter, 2, 1) X(type, reserved8E, 2, 1) \
    X(type, verticalPitch, 2, 1) X(type, bodyKickOffset, 2, 1) X(type, verticalRoll, 2, 1) \
    X(type, reserved96, 2, 1) X(type, verticalMotionState, 2, 1) X(type, verticalMotionTimer, 2, 1) \
    X(type, verticalMotionRate, 2, 1) X(type, verticalTargetY, 2, 1) X(type, headingAngle, 4, 1) \
    X(type, speed, 4, 1) X(type, acceleration, 4, 1) X(type, activeFlag, 2, 1) \
    X(type, modelIndex, 2, 1) X(type, initializedFlag, 4, 1) X(type, trackHeading, 4, 1) \
    X(type, facingBackwards, 2, 1) X(type, padBA, 1, 2)

#define HUMAN_FIELDS(X) COMMON_CAR_FIELDS(X, PlayerCarRuntime) \
    X(PlayerCarRuntime, drive.steerHoldFrames, 2, 1) X(PlayerCarRuntime, drive.gripLossTimer, 2, 1) X(PlayerCarRuntime, drive.autoShiftCooldown, 4, 1) \
    X(PlayerCarRuntime, drive.accelPos, 4, 1) X(PlayerCarRuntime, drive.roadGrade, 4, 1) X(PlayerCarRuntime, drive.brakePos, 4, 1) \
    X(PlayerCarRuntime, drive.driveBoostTimer, 4, 1) X(PlayerCarRuntime, drive.standingStartSpin, 4, 1) X(PlayerCarRuntime, drive.steerPos, 4, 1) \
    X(PlayerCarRuntime, drive.shiftSoundLevel, 4, 1) X(PlayerCarRuntime, drive.reserved24, 4, 1) X(PlayerCarRuntime, drive.launchThresholdIndex, 4, 1) \
    X(PlayerCarRuntime, drive.engineLoad, 2, 1) X(PlayerCarRuntime, drive.drivetrainCoupled, 2, 1) X(PlayerCarRuntime, drive.gearDisp, 2, 1) \
    X(PlayerCarRuntime, drive.steeringGrip, 2, 1) X(PlayerCarRuntime, drive.clutch, 2, 1) X(PlayerCarRuntime, drive.shiftSpeedDelta, 2, 1) \
    X(PlayerCarRuntime, drive.jumpTimer, 2, 1) X(PlayerCarRuntime, drive.dragScale, 2, 1) X(PlayerCarRuntime, drive.shiftRpmDelta, 2, 1) \
    X(PlayerCarRuntime, drive.bodyLiftOffset, 2, 1) X(PlayerCarRuntime, drive.trackCurveMode, 2, 1) X(PlayerCarRuntime, drive.trackCurveBias, 2, 1) \
    X(PlayerCarRuntime, drive.coastFrames, 4, 1) X(PlayerCarRuntime, drive.launchEnergy, 4, 1) X(PlayerCarRuntime, drive.steeringLoadAngle, 4, 1) \
    X(PlayerCarRuntime, drive.spinRate, 4, 1) X(PlayerCarRuntime, drive.launchDirection, 4, 1) X(PlayerCarRuntime, drive.launchHeading, 4, 1) \
    X(PlayerCarRuntime, drive.launchSpeed, 4, 1) X(PlayerCarRuntime, drive.yawOffset, 4, 1) X(PlayerCarRuntime, drive.reserved64, 4, 1) \
    X(PlayerCarRuntime, drive.standingStartBounceY, 4, 1) X(PlayerCarRuntime, drive.standingStartBounceX, 4, 1) X(PlayerCarRuntime, drive.reserved70, 2, 1) \
    X(PlayerCarRuntime, drive.reserved72, 2, 1) X(PlayerCarRuntime, drive.manual, 2, 1) X(PlayerCarRuntime, drive.gear, 2, 1) \
    X(PlayerCarRuntime, drive.engineRpm, 4, 1) X(PlayerCarRuntime, drive.shiftTargetRpm, 4, 1) X(PlayerCarRuntime, drive.shiftTargetSpeed, 4, 1) \
    X(PlayerCarRuntime, drive.launchEnergyThreshold, 4, 1) X(PlayerCarRuntime, drive.steeringGripResponse, 4, 1) X(PlayerCarRuntime, drive.speedScale, 4, 1) \
    X(PlayerCarRuntime, drive.targetHeading, 4, 1) X(PlayerCarRuntime, drive.drivetrainTorque, 4, 1) X(PlayerCarRuntime, drive.motionState, 4, 1) \
    X(PlayerCarRuntime, drive.acceleratorLatch, 2, 1) X(PlayerCarRuntime, drive.brakeLatch, 2, 1) X(PlayerCarRuntime, drive.acceleratorInput.value, 2, 1) \
    X(PlayerCarRuntime, drive.brakeInput, 2, 1) X(PlayerCarRuntime, drive.racePosition, 2, 1) X(PlayerCarRuntime, drive.hudLapHighlightRow, 2, 1) \
    X(PlayerCarRuntime, previousTrackPointIndex, 4, 1) X(PlayerCarRuntime, lap, 2, 1) X(PlayerCarRuntime, reserved16A, 2, 1) \
    X(PlayerCarRuntime, lapTimes.words, 4, 12)
HUMAN_FIELDS(FRAME_WIDTH)
static const FrameField human_fields[] = { HUMAN_FIELDS(FRAME_FIELD) };
enum { HUMAN_FIELDS_BYTES = 0 HUMAN_FIELDS(FRAME_BYTES) };

#define RIVAL_FIELDS(X) COMMON_CAR_FIELDS(X, GameCarRuntime) \
    X(GameCarRuntime, aiEnabled, 4, 1) X(GameCarRuntime, reservedC0, 4, 1) X(GameCarRuntime, reservedC4, 4, 1) \
    X(GameCarRuntime, worldVelocityX, 4, 1) X(GameCarRuntime, reservedCC, 4, 1) X(GameCarRuntime, worldVelocityZ, 4, 1) \
    X(GameCarRuntime, reservedD4, 4, 1) X(GameCarRuntime, reservedD8, 4, 1) X(GameCarRuntime, reservedDC, 4, 1) \
    X(GameCarRuntime, reservedE0, 4, 1) X(GameCarRuntime, renderDepth, 4, 1) X(GameCarRuntime, reservedE8, 2, 1) \
    X(GameCarRuntime, reservedEA, 2, 1) X(GameCarRuntime, targetYaw, 4, 1) X(GameCarRuntime, slideInput, 4, 1) \
    X(GameCarRuntime, yawRate, 4, 1) X(GameCarRuntime, reservedF8, 4, 1) X(GameCarRuntime, initialLateralOffset, 4, 1) \
    X(GameCarRuntime, racingLineHintIndex, 4, 1) X(GameCarRuntime, avoidanceActive, 2, 1) X(GameCarRuntime, pad106, 1, 2) \
    X(GameCarRuntime, baseBodyYaw, 4, 1) X(GameCarRuntime, nearbyCarCount, 2, 1) X(GameCarRuntime, reserved10E, 2, 1) \
    X(GameCarRuntime, reserved110, 2, 1) X(GameCarRuntime, reserved112, 2, 1) X(GameCarRuntime, reserved114, 2, 1) \
    X(GameCarRuntime, reserved116, 2, 1) X(GameCarRuntime, gridTargetProgress, 2, 1) X(GameCarRuntime, reserved11A, 2, 1) \
    X(GameCarRuntime, aiLateralOffset, 2, 1) X(GameCarRuntime, avoidanceTargetOffset, 2, 1) X(GameCarRuntime, avoidanceStep, 2, 1) \
    X(GameCarRuntime, rivalModelId, 2, 1) X(GameCarRuntime, targetSpeed, 2, 1) X(GameCarRuntime, accelerationStep, 2, 1) \
    X(GameCarRuntime, boostAccelerationThreshold, 2, 1) X(GameCarRuntime, collisionBoostDuration, 2, 1) X(GameCarRuntime, boostAcceleration, 2, 1) \
    X(GameCarRuntime, boostTimer, 2, 1) X(GameCarRuntime, accelerationLimit, 2, 1) X(GameCarRuntime, minimumSpeed, 2, 1) \
    X(GameCarRuntime, engineRpm, 4, 1) X(GameCarRuntime, speedKeyIndex, 2, 1) X(GameCarRuntime, slideActive, 2, 1) \
    X(GameCarRuntime, reserved13C, 4, 1) X(GameCarRuntime, pad140, 1, 12) X(GameCarRuntime, reserved14C, 1, 1) \
    X(GameCarRuntime, reserved14D, 1, 1) X(GameCarRuntime, reserved14E, 1, 1) X(GameCarRuntime, reserved14F, 1, 1) \
    X(GameCarRuntime, reserved150, 4, 1) X(GameCarRuntime, reserved154, 4, 1) X(GameCarRuntime, pad158, 1, 4) \
    X(GameCarRuntime, acceleratorInput, 2, 1) X(GameCarRuntime, brakeInput, 2, 1) X(GameCarRuntime, reserved160, 2, 1) \
    X(GameCarRuntime, reserved162, 2, 1) X(GameCarRuntime, previousTrackPointIndex, 4, 1) X(PlayerCarRuntime, lap, 2, 1) \
    X(PlayerCarRuntime, reserved16A, 2, 1) X(PlayerCarRuntime, lapTimes.words, 4, 12)
RIVAL_FIELDS(FRAME_WIDTH)
static const FrameField rival_fields[] = { RIVAL_FIELDS(FRAME_FIELD) };
enum { RIVAL_FIELDS_BYTES = 0 RIVAL_FIELDS(FRAME_BYTES) };

_Static_assert(HUMAN_FIELDS_BYTES == RIVAL_FIELDS_BYTES, "checkpoint car payloads differ");
_Static_assert(RACE_FRAME_WIRE_SIZE == 1 + FRAME_FIELDS_BYTES + DRIVER_SEAT_LIMIT *
               (DRIVER_FIELDS_BYTES + HUMAN_FIELDS_BYTES), "checkpoint wire size changed");

static uint8_t *WriteFrameFields(uint8_t *wire, const void *object,
                                const FrameField *fields, size_t count) {
    for (size_t field = 0; field < count; ++field) {
        for (unsigned element = 0; element < fields[field].count; ++element) {
            const uint8_t *value = (const uint8_t *)object + fields[field].offset + element * fields[field].width;
            uint32_t bits = 0;
            if (fields[field].width == 4) memcpy(&bits, value, 4);
            else if (fields[field].width == 2) { uint16_t half; memcpy(&half, value, 2); bits = half; }
            else bits = *value;
            for (unsigned byte = 0; byte < fields[field].width; ++byte) *wire++ = (uint8_t)(bits >> (8 * byte));
        }
    }
    return wire;
}

static const uint8_t *ReadFrameFields(const uint8_t *wire, void *object,
                                     const FrameField *fields, size_t count) {
    for (size_t field = 0; field < count; ++field) {
        for (unsigned element = 0; element < fields[field].count; ++element) {
            uint8_t *value = (uint8_t *)object + fields[field].offset + element * fields[field].width;
            uint32_t bits = 0;
            for (unsigned byte = 0; byte < fields[field].width; ++byte) bits |= (uint32_t)*wire++ << (8 * byte);
            if (fields[field].width == 4) memcpy(value, &bits, 4);
            else if (fields[field].width == 2) { uint16_t half = (uint16_t)bits; memcpy(value, &half, 2); }
            else *value = (uint8_t)bits;
        }
    }
    return wire;
}

#define FIELD_COUNT(fields) (sizeof(fields) / sizeof(*(fields)))
int EncodeRaceFrame(const RaceSim *race, uint8_t *wire, size_t size) {
    RaceFrame frame;
    if (!wire || size != RACE_FRAME_WIRE_SIZE || !SaveRaceFrame(race, &frame)) return 0;
    uint8_t *cursor = wire;
    *cursor++ = RACE_FRAME_WIRE_VERSION;
    cursor = WriteFrameFields(cursor, &frame, frame_fields, FIELD_COUNT(frame_fields));
    for (int seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) {
        const DriverFrame *driver = &frame.drivers[seat];
        cursor = WriteFrameFields(cursor, driver, driver_fields, FIELD_COUNT(driver_fields));
        cursor = driver->rival
            ? WriteFrameFields(cursor, AsConstRivalCar(&driver->car), rival_fields, FIELD_COUNT(rival_fields))
            : WriteFrameFields(cursor, &driver->car, human_fields, FIELD_COUNT(human_fields));
    }
    return 1;
}

int DecodeRaceFrame(const RaceSim *race, const uint8_t *wire, size_t size, RaceFrame *out) {
    if (!race || !wire || !out || size != RACE_FRAME_WIRE_SIZE || wire[0] != RACE_FRAME_WIRE_VERSION) return 0;
    RaceFrame frame = {.track = race->route.points, .events = race->events};
    const uint8_t *cursor = ReadFrameFields(wire + 1, &frame, frame_fields, FIELD_COUNT(frame_fields));
    for (int seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) {
        DriverFrame *driver = &frame.drivers[seat];
        cursor = ReadFrameFields(cursor, driver, driver_fields, FIELD_COUNT(driver_fields));
        if (driver->rival != race->drivers[seat].rival) return 0;
        cursor = driver->rival
            ? ReadFrameFields(cursor, AsRivalCar(&driver->car), rival_fields, FIELD_COUNT(rival_fields))
            : ReadFrameFields(cursor, &driver->car, human_fields, FIELD_COUNT(human_fields));
    }
    if (!ValidRaceFrame(race, &frame)) return 0;
    *out = frame;
    return 1;
}
