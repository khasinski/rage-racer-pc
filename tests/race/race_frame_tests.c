#include "game/race_sim.h"
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

static int TestFrameCodec(const RaceSim *source) {
    uint8_t guarded[RACE_FRAME_WIRE_SIZE + 2];
    memset(guarded, 0xA5, sizeof(guarded));
    uint8_t *wire = guarded + 1;
    CHECK(EncodeRaceFrame(source, wire, RACE_FRAME_WIRE_SIZE));
    CHECK(guarded[0] == 0xA5 && guarded[sizeof(guarded) - 1] == 0xA5);
    const uint8_t header[] = {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             100, 0, 0, 0, 0, 0, 0, 0, 80, 0, 0, 0, SIM_RACING, 0, 0, 0};
    CHECK(memcmp(wire, header, sizeof(header)) == 0);
    const uint8_t negativeX[] = {0x2E, 0xFB, 0xFF, 0xFF};
    CHECK(memcmp(wire + 29 + 116, negativeX, 4) == 0);
    CHECK(wire[29 + 116 + 124] == 0xE0 && wire[29 + 116 + 125] == 0xFF);
    CHECK(wire[29 + 116 + 130] == 0xCD && wire[29 + 116 + 131] == 0xAB);
    uint8_t encoded[RACE_FRAME_WIRE_SIZE];
    memcpy(encoded, wire, sizeof(encoded));
    GameTrackPoint otherPoints[3] = {0};
    RaceSim receiver = *source;
    receiver.route.points = otherPoints;
    RaceFrame frame;
    CHECK(DecodeRaceFrame(&receiver, wire, sizeof(encoded), &frame));
    CHECK(frame.track == otherPoints && frame.events == receiver.events);
    receiver.drivers[0].car.x = 900;
    CHECK(RestoreRaceFrame(&receiver, &frame));
    RaceSim expected = *source; expected.route.points = otherPoints;
    CHECK(memcmp(&receiver, &expected, sizeof(receiver)) == 0);
    uint8_t repeated[RACE_FRAME_WIRE_SIZE];
    CHECK(EncodeRaceFrame(&receiver, repeated, sizeof(repeated)));
    CHECK(memcmp(repeated, encoded, sizeof(encoded)) == 0);
    memset(&frame, 0xA5, sizeof(frame));
    const RaceFrame untouched = frame;
    for (size_t length = 0; length < sizeof(encoded); ++length) {
        CHECK(!DecodeRaceFrame(&receiver, wire, length, &frame));
        CHECK(memcmp(&frame, &untouched, sizeof(frame)) == 0);
    }
    CHECK(!DecodeRaceFrame(&receiver, wire, sizeof(encoded) + 1, &frame));
    for (unsigned version = 0; version <= 255; ++version) {
        if (version == RACE_FRAME_WIRE_VERSION) continue;
        wire[0] = (uint8_t)version;
        CHECK(!DecodeRaceFrame(&receiver, wire, sizeof(encoded), &frame));
        CHECK(memcmp(&frame, &untouched, sizeof(frame)) == 0);
    }
    memcpy(wire, encoded, sizeof(encoded));
    wire[29 + (DRIVER_SEAT_LIMIT - 2) * 528 + 116 + 306] = 7; // Late human's gear.
    CHECK(!DecodeRaceFrame(&receiver, wire, sizeof(encoded), &frame));
    CHECK(memcmp(&frame, &untouched, sizeof(frame)) == 0);
    memcpy(wire, encoded, sizeof(encoded));
    wire[29 + (DRIVER_SEAT_LIMIT - 1) * 528 + 48] = 0; // Wrong AI interpretation.
    CHECK(!DecodeRaceFrame(&receiver, wire, sizeof(encoded), &frame));
    CHECK(memcmp(&frame, &untouched, sizeof(frame)) == 0);
    memset(wire, 0xA5, sizeof(encoded));
    receiver.drivers[0].car.drive.gear = 7;
    CHECK(!EncodeRaceFrame(&receiver, wire, sizeof(encoded)));
    for (size_t byte = 0; byte < sizeof(encoded); ++byte) CHECK(wire[byte] == 0xA5);
    CHECK(!EncodeRaceFrame(NULL, wire, sizeof(encoded)));
    CHECK(!EncodeRaceFrame(source, NULL, sizeof(encoded)));
    CHECK(!EncodeRaceFrame(source, wire, sizeof(encoded) - 1));
    CHECK(!DecodeRaceFrame(NULL, encoded, sizeof(encoded), &frame));
    CHECK(!DecodeRaceFrame(source, NULL, sizeof(encoded), &frame));
    CHECK(!DecodeRaceFrame(source, encoded, sizeof(encoded), NULL));
    return 0;
}

int main(void) {
    const GameTrackPoint points[3] = {0};
    RaceSim race = {.route = {.points = points, .count = 3, .length = 3000},
        .laps = 1, .phase = SIM_RACING, .tick = 100, .elapsed = 80};
    for (int seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) {
        race.drivers[seat].variant = -1;
        if (seat != 0 && seat < DRIVER_SEAT_LIMIT - 2) continue;
        SimDriver *driver = &race.drivers[seat];
        memset(&driver->car, 0xA5, sizeof(driver->car));
        driver->car.trackPointIndex = 0;
        driver->car.activeFlag = 0;
        driver->car.facingBackwards = 0;
        driver->car.verticalMotionState = CAR_VERTICAL_GROUNDED;
        driver->car.drive.motionState = CAR_MOTION_DRIVING;
        driver->car.drive.acceleratorInput.value = 256;
        driver->car.drive.brakeInput = 128;
        driver->car.drive.acceleratorLatch = driver->car.drive.brakeLatch = 1;
        driver->status = SIM_DRIVING;
        driver->car.modelIndex = 23;
        driver->car.lap = 1;
        driver->car.previousTrackPointIndex = 2;
        driver->car.drive.gear = 1;
        driver->car.drive.manual = 1;
        driver->input.throttle = 256;
        driver->random = 42;
        driver->inputTick = driver->stepTick = 98;
    }
    SimDriver *ai = &race.drivers[DRIVER_SEAT_LIMIT - 1];
    ai->rival = 1; ai->rivalSlot = 3;
    // These player fields overlap AI state and must not be validated as a gearbox.
    ai->car.drive.gear = 99; ai->car.drive.manual = 99;
    AsRivalCar(&ai->car)->acceleratorInput = 256;
    AsRivalCar(&ai->car)->brakeInput = 128;
    race.drivers[0].car.x = -1234;
    race.drivers[0].car.velocityX = -32;
    race.drivers[0].car.motionTimer = 0xABCD;
    race.drivers[0].car.drive.shiftTargetSpeed = -250;
    race.drivers[0].car.drive.speedScale = INT32_MIN;
    race.drivers[0].car.lapTimes.words[11] = 0x11223344;
    AsRivalCar(&ai->car)->engineRpm = 4200;
    AsRivalCar(&ai->car)->worldVelocityX = -0x1234567;
    CHECK(TestFrameCodec(&race) == 0);
    RaceFrame frame;
    CHECK(SaveRaceFrame(&race, &frame));
    const RaceSim original = race;
    const RaceFrame saved = frame;
    CHECK(ValidRaceFrame(&race, &frame));
    CHECK(memcmp(&race, &original, sizeof(race)) == 0);
    CHECK(memcmp(&frame, &saved, sizeof(frame)) == 0);
    race.drivers[0].car.x = 500;
    race.drivers[0].random = 19;
    CHECK(RestoreRaceFrame(&race, &frame));
    CHECK(memcmp(&race, &original, sizeof(race)) == 0);

    for (int problem = 0; problem < 50; ++problem) {
        frame = saved;
        frame.drivers[0].car.x += 200;
        DriverFrame *human = &frame.drivers[DRIVER_SEAT_LIMIT - 2];
        DriverFrame *rival = &frame.drivers[DRIVER_SEAT_LIMIT - 1];
        switch (problem) {
        case 0: human->car.drive.gear = -1; break;
        case 1: human->car.drive.gear = 7; break;
        case 2: human->car.drive.motionState = (CarMotionState)99; break;
        case 3: human->car.drive.manual = 0; break;
        case 4: human->car.drive.acceleratorInput.value = 257; break;
        case 5: human->car.drive.brakeInput = -1; break;
        case 6: human->car.drive.acceleratorLatch = 3; break;
        case 7: human->car.drive.brakeLatch = -1; break;
        case 8: human->car.previousTrackPointIndex = 3; break;
        case 9: human->car.trackPointIndex = -1; break;
        case 10: human->car.verticalMotionState = 99; break;
        case 11: human->car.facingBackwards = 2; break;
        case 12: human->car.modelIndex++; break;
        case 13: human->car.activeFlag = -1; break;
        case 14: human->status = SIM_RETIRED; break;
        case 15: human->place = 1; break;
        case 16: human->wrongWayFrames = -1; break;
        case 17: human->crashed = 2; break;
        case 18: human->step.motionFinished = 2; break;
        case 19: human->step.landingFrames = -1; break;
        case 20: human->lapTicks[5] = frame.elapsed + 1; break;
        case 21: human->inputTick = frame.tick + 1; break;
        case 22: human->stepTick = frame.tick + 1; break;
        case 23: human->lapStarted = frame.elapsed + 1; break;
        case 24: human->finishTick = frame.elapsed + 1; break;
        case 25: human->input.shiftUp = 2; break;
        case 26: human->car.lap = frame.laps + 2; break;
        case 27: human->variant++; break;
        case 28: rival->rivalSlot++; break;
        case 29: rival->rival = 0; break;
        case 30: rival->status = (SimDriverStatus)99; break;
        case 31: AsRivalCar(&rival->car)->acceleratorInput = -1; break;
        case 32: AsRivalCar(&rival->car)->brakeInput = 257; break;
        case 33: frame.phase = (SimRacePhase)99; break;
        case 34: frame.elapsed = frame.tick + 1; break;
        case 35: frame.finishCount = 1; break;
        case 36: frame.finishCount = -1; break;
        case 37: frame.finishCount = DRIVER_SEAT_LIMIT + 1; break;
        case 38: frame.track = points + 1; break;
        case 39: frame.events = points; break;
        case 40: frame.phase = SIM_FINISHED; break;
        case 41: frame.phase = SIM_COUNTDOWN; frame.elapsed = 0; break;
        case 43: frame.laps++; break;
        case 44: frame.reverse++; break;
        case 45: frame.phase = SIM_SETUP; break;
        case 46: frame.countdown = 1; break;
        case 47: human->input.throttle = 257; break;
        case 48: frame.drivers[1].status = SIM_DRIVING; break;
        case 49: human->car.drive.gear = 0; break;
        default:
            frame.finishCount = 2;
            frame.drivers[0].status = human->status = SIM_DRIVER_FINISHED;
            frame.drivers[0].car.activeFlag = human->car.activeFlag = -1;
            frame.drivers[0].place = human->place = 1;
            break;
        }
        CHECK(!ValidRaceFrame(&race, &frame));
        CHECK(!RestoreRaceFrame(&race, &frame));
        CHECK(memcmp(&race, &original, sizeof(race)) == 0);
    }
    frame = saved;
    race.drivers[0].car.drive.gear = 7;
    CHECK(!SaveRaceFrame(&race, &frame));
    CHECK(memcmp(&frame, &saved, sizeof(frame)) == 0);
    race = original;
    frame = saved;
    frame.finishCount = 1;
    frame.drivers[0].status = SIM_DRIVER_FINISHED;
    frame.drivers[0].car.activeFlag = -1;
    frame.drivers[0].place = 1;
    CHECK(RestoreRaceFrame(&race, &frame));
    CHECK(race.finishCount == 1 && race.drivers[0].place == 1);
    // An aborted countdown legitimately retains its unused countdown value.
    frame.phase = SIM_FINISHED; frame.elapsed = 0; frame.finishCount = 0; frame.countdown = 37;
    for (int seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) {
        if (frame.drivers[seat].status == SIM_EMPTY) continue;
        frame.drivers[seat].status = SIM_RETIRED;
        frame.drivers[seat].car.activeFlag = -1;
        frame.drivers[seat].place = 0;
    }
    CHECK(RestoreRaceFrame(&race, &frame));
    CHECK(SaveRaceFrame(&race, &frame));
    CHECK(!ValidRaceFrame(NULL, &frame) && !ValidRaceFrame(&race, NULL));
    CHECK(!SaveRaceFrame(NULL, &frame) && !SaveRaceFrame(&race, NULL));
    CHECK(!RestoreRaceFrame(NULL, &frame) && !RestoreRaceFrame(&race, NULL));
    puts("race_frame: pure owned checkpoint validation and atomic restoration pass");
    return 0;
}
