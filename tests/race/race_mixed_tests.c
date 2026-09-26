#include "game/race_sim.h"
#include "../car/driver_fixture.h"
#include <stdio.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)

int main(void) {
    const GameTrackPoint points[3] = {
        {.x = 0, .segmentLength = 1000, .leftHalfWidth = 300, .rightHalfWidth = 300},
        {.x = 1000, .segmentLength = 1000, .leftHalfWidth = 300, .rightHalfWidth = 300},
        {.x = 2000, .segmentLength = 1000, .leftHalfWidth = 300, .rightHalfWidth = 300},
    };
    const TrackRoute route = {.points = points, .count = 3, .length = 3000};
    TrackEventData events = {0};
    for (int reverse = 0; reverse < 2; reverse++) {
        events.aiSpeedKeys[reverse][1].progress = 300;
        for (int key = 0; key < 2; key++) {
            for (int slot = 0; slot < 4; slot++) {
                events.aiSpeedKeys[reverse][key].slotTargetSpeeds[slot] = 100;
            }
        }
    }
    const DriverHull hull = {g_PlayerHullPoints, g_OpponentHullCorners};
    const LaunchSpeedThreshold threshold = {30000, 30000};
    const TrackRivalStart start = {.x = 200, .trackPointIndex = 0};
    const TrackRivalAiConfig config = {.speed = 100, .accelerationStep = 5,
        .minimumSpeed = 60, .initialEngineRpm = 1200};
    for (int reverse = 0; reverse < 2; reverse++) {
        for (int model = 0; model < TRACK_RIVAL_COUNT; model++) {
            events.rivalAiConfigs[reverse][model] = config;
        }
    }
    events.rivalAiConfigs[0][4].speed = 160;
    events.rivalAiConfigs[0][4].accelerationStep = 9;
    events.rivalAiConfigs[0][6].speed = 80;
    events.rivalAiConfigs[0][6].accelerationStep = 3;
    PlayerCarRuntime fixture;
    GameCarSpec spec;
    CarPerformance engine = {0};
    PrepareDriver(&fixture, &spec, &engine);
    const DriverInput input = {.steering.mode = STEERING_DIGITAL, .throttle = 256};
    RaceSim race, baseline, other;
    CHECK(InitRaceSim(&race, &route, &events, 1, 0));
    baseline = race;
    CHECK(!AddRaceRival(&race, 0, 11, &hull, g_CarCollisionCorners, &start, 0, 3));
    CHECK(!AddRaceRival(&race, 12, 0, &hull, g_CarCollisionCorners, &start, 0, 3));
    CHECK(!AddRaceRival(&race, 0, 0, NULL, g_CarCollisionCorners, &start, 0, 3));
    CHECK(!AddRaceRival(&race, 0, 0, &hull, NULL, &start, 0, 3));
    CHECK(!AddRaceRival(&race, 0, 0, &hull, g_CarCollisionCorners, NULL, 0, 3));
    TrackRivalStart inactive = start;
    inactive.activeFlag = -1;
    CHECK(!AddRaceRival(&race, 0, 0, &hull, g_CarCollisionCorners, &inactive, 0, 3));
    CHECK(memcmp(&race, &baseline, sizeof race) == 0);
    CHECK(InitRaceSim(&other, &route, NULL, 1, 0));
    CHECK(!AddRaceRival(&other, 0, 0, &hull, g_CarCollisionCorners, &start, 0, 3));
    CHECK(AddRaceDriver(&other, 0, &spec, &hull, g_CarCornerOffsets,
        &threshold, &inactive, 0, 1, 23, 123));
    CHECK(other.drivers[0].car.activeFlag != -1);
    for (int slot = 0; slot < 4; slot++) {
        if (slot % 2 == 0) {
            CHECK(AddRaceDriver(&race, slot, &spec, &hull, g_CarCornerOffsets,
                &threshold, &start, 0, 1, 23, 123 + slot));
            CHECK(SetRaceInput(&race, slot, &input));
        } else {
            CHECK(AddRaceRival(&race, slot, slot / 2, &hull, g_CarCollisionCorners, &start, 0, 3 + slot));
        }
    }
    baseline = race;
    CHECK(!AddRaceRival(&race, 4, 0, &hull, g_CarCollisionCorners, &start, 0, 3));
    CHECK(!SetRaceInput(&race, 1, &input));
    CHECK(memcmp(&race, &baseline, sizeof race) == 0);
    CHECK(race.drivers[1].rival && race.drivers[1].rivalSlot == 0);
    CHECK(race.drivers[3].rival && race.drivers[3].rivalSlot == 1);
    CHECK(AsRivalCar(&race.drivers[1].car)->targetSpeed == 1168);
    CHECK(AsRivalCar(&race.drivers[1].car)->accelerationStep == 9);
    CHECK(AsRivalCar(&race.drivers[3].car)->targetSpeed == 584);
    CHECK(AsRivalCar(&race.drivers[3].car)->accelerationStep == 3);
    CHECK(memcmp(race.drivers[1].corners, g_OpponentHullCorners,
        sizeof(race.drivers[1].corners)) == 0);
    CHECK(memcmp(race.drivers[1].rivalCorners, g_CarCollisionCorners,
        sizeof(race.drivers[1].rivalCorners)) == 0);
    /* Movement order must not change the traffic seen by an AI. Place cars
     * apart so stable collision ordering does not affect this comparison. */
    baseline = race;
    baseline.drivers[0].car.x = 800;
    baseline.drivers[2].car.x = 1400;
    CHECK(RetireRaceDriver(&baseline, 3));
    other = baseline;
    other.drivers[0] = baseline.drivers[1];
    other.drivers[1] = baseline.drivers[0];
    CHECK(StartRaceSim(&baseline, 0) && StartRaceSim(&other, 0));
    CHECK(StepRaceSim(&baseline) && StepRaceSim(&other));
    CHECK(StepRaceSim(&baseline) && StepRaceSim(&other));
    CHECK(baseline.drivers[1].car.collisionFlag == 0);
    CHECK(memcmp(&baseline.drivers[1], &other.drivers[0], sizeof(SimDriver)) == 0);
    const SimDriver before = race.drivers[1];
    CHECK(StartRaceSim(&race, 4));
    CHECK(!AddRaceRival(&race, 4, 2, &hull, g_CarCollisionCorners, &start, 0, 3));
    for (int tick = 0; tick < 4; tick++) CHECK(StepRaceSim(&race));
    CHECK(memcmp(&race.drivers[1], &before, sizeof before) == 0);
    CHECK(race.drivers[0].car.drive.engineRpm > 0);
    CHECK(race.phase == SIM_RACING && race.elapsed == 0);
    baseline = other = race;
    for (int tick = 0; tick < 40; tick++) {
        RaceSim restored = race;
        CHECK(StepRaceSim(&race));
        CHECK(StepRaceSim(&other));
        CHECK(StepRaceSim(&restored));
        CHECK(memcmp(&race, &restored, sizeof race) == 0);
        CHECK(memcmp(&race, &other, sizeof race) == 0);
        if (tick == 1) {
            /* Both human/AI and AI/AI pairs are resolved after movement. */
            for (int slot = 0; slot < 4; slot++) CHECK(race.drivers[slot].car.collisionFlag == 1);
        }
    }
    CHECK(race.drivers[1].car.x != before.car.x);
    CHECK(race.drivers[3].car.x != before.car.x);
    CHECK(race.drivers[1].random == 0 && race.drivers[3].random == 0);
    CHECK(race.drivers[4].status == SIM_EMPTY);
    /* AI are ordinary race participants: humans retiring does not stop them. */
    CHECK(RetireRaceDriver(&race, 0) && RetireRaceDriver(&race, 2));
    CHECK(StepRaceSim(&race) && race.phase == SIM_RACING);
    CHECK(RetireRaceDriver(&race, 3));
    race.drivers[1].car.lap = 1;
    race.drivers[1].car.progressA = 6000;
    race.drivers[1].lapStarted = 2;
    CHECK(StepRaceSim(&race));
    CHECK(race.phase == SIM_FINISHED && race.finishCount == 1);
    CHECK(race.drivers[1].status == SIM_DRIVER_FINISHED && race.drivers[1].place == 1);
    CHECK(race.drivers[1].finishTick == race.elapsed);
    CHECK(race.drivers[1].lapTicks[0] == race.elapsed - 2);
    baseline = race;
    CHECK(!StepRaceSim(&race));
    CHECK(memcmp(&race, &baseline, sizeof race) == 0);
    return 0;
}
