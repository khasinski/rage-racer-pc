#include "driver_fixture.h"
#include "game/car_motion_internal.h"
#include "game/car_track_internal.h"
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

int main(void) {
    const GameTrackPoint points[3] = {
        {.x = 0, .segmentLength = 1000, .leftHalfWidth = 100, .rightHalfWidth = 100},
        {.x = 1000, .segmentLength = 1000, .leftHalfWidth = 100, .rightHalfWidth = 100},
        {.x = 2000, .segmentLength = 1000, .leftHalfWidth = 100, .rightHalfWidth = 100},
    };
    GameTrackPoint raised[3];
    memcpy(raised, points, sizeof(raised));
    for (int i = 0; i < 3; i++) raised[i].y = 50;
    const TrackRoute route = {.points = points, .count = 3, .length = 3000};
    const TrackRoute otherRoute = {.points = raised, .count = 3, .length = 3000};
    const CarHullPoint corners[4] = {{-8, -8}, {8, -8}, {-8, 8}, {8, 8}};
    const LaunchSpeedThreshold threshold = {.initial = 30000, .sustain = 30000};
    GameCarSpec spec, otherSpec;
    CarPerformance engine = {0}, otherEngine = {0};
    PlayerCarRuntime first, second;
    PrepareDriver(&first, &spec, &engine);
    PrepareDriver(&second, &otherSpec, &otherEngine);
    DriverContext context = {.spec = &spec, .performance = &engine, .route = &route,
        .launchThreshold = &threshold, .corners = corners,
        .drive = {.racing = 1, .started = 1, .digitalSteering = 1}};
    DriverContext other = context;
    other.spec = &otherSpec;
    other.performance = &otherEngine;
    other.route = &otherRoute;
    const DriverInput input = {.steering.mode = STEERING_DIGITAL, .throttle = 256};
    /* Full human fields: all cars move, pair detection/response, then finish. */
    PlayerCarRuntime fieldA[2], fieldB[2], baseline[2];
    const TrackRivalStart position = {.x = 200, .z = 0, .trackPointIndex = 0};
    const DriverStart start = {.route = &route, .position = &position,
        .manual = 1, .modelIndex = 23};
    DriverStart otherStart = start;
    otherStart.route = &otherRoute;
    for (int i = 0; i < 2; i++) {
        PrepareDriver(&fieldA[i], &spec, &engine);
        PrepareDriver(&fieldB[i], &otherSpec, &otherEngine);
        InitDriver(&fieldA[i], &spec, &engine, &start);
        InitDriver(&fieldB[i], &otherSpec, &otherEngine, &otherStart);
        baseline[i] = fieldA[i];
    }
    DriverSeat seatsA[3] = {
        {.car = &fieldA[0], .context = &context, .input = input, .hull = {0}, .random = 123},
        {0},
        {.car = &fieldA[1], .context = &context, .input = input, .hull = {0}, .random = 456},
    };
    const CarHullPoint hullPoints[6] = {
        {-32, 64}, {32, 64}, {-24, -72}, {24, -72}, {-32, 16}, {32, 16},
    };
    seatsA[0].hull = seatsA[2].hull = (DriverHull){hullPoints, corners};
    DriverSeat seatsB[3] = {seatsA[0], {0}, seatsA[2]};
    seatsB[0].car = &fieldB[0]; seatsB[2].car = &fieldB[1];
    seatsB[0].context = seatsB[2].context = &other;
    DriverSeat baselineSeats[3] = {seatsA[0], {0}, seatsA[2]};
    baselineSeats[0].car = &baseline[0]; baselineSeats[2].car = &baseline[1];
    /* Validation is atomic, including a duplicated car pointer. */
    DriverSeat missingHull[1] = {seatsA[0]};
    missingHull[0].hull.corners = NULL;
    CHECK(!StepDriverField(missingHull, 1));
    CHECK(memcmp(fieldA, baseline, sizeof(fieldA)) == 0);
    DriverSeat duplicate[2] = {seatsA[0], seatsA[0]};
    CHECK(StepDriverField(duplicate, 2) == 0);
    CHECK(memcmp(fieldA, baseline, sizeof(fieldA)) == 0);
    CHECK(StepDriverField(seatsA, DRIVER_SEAT_LIMIT + 1) == 0);
    CHECK(StepDriverField(NULL, 1) == 0);
    DriverContext invalid = context;
    invalid.route = &otherRoute;
    DriverSeat mixed[2] = {seatsA[0], seatsA[2]};
    mixed[1].context = &invalid;
    CHECK(StepDriverField(mixed, 2) == 0);
    CHECK(memcmp(fieldA, baseline, sizeof(fieldA)) == 0);
    for (int i = 0; i < 10; i++) {
        CHECK(StepDriverField(seatsA, 3) == 1);
        CHECK(StepDriverField(seatsB, 3) == 1);
        CHECK(StepDriverField(baselineSeats, 3) == 1);
        CHECK(memcmp(fieldA, baseline, sizeof(fieldA)) == 0);
        CHECK(seatsA[0].random == baselineSeats[0].random);
        CHECK(seatsA[2].random == baselineSeats[2].random);
        CHECK(seatsA[0].crashed == 1 && seatsA[2].crashed == 1);
        CHECK(fieldA[0].collisionFlag == 1 && fieldA[1].collisionFlag == 1);
    }
    /* Restoring car and per-seat RNG reproduces the whole field step. */
    memcpy(baseline, fieldA, sizeof(baseline));
    baselineSeats[0] = seatsA[0]; baselineSeats[2] = seatsA[2];
    baselineSeats[0].car = &baseline[0]; baselineSeats[2].car = &baseline[1];
    CHECK(StepDriverField(seatsA, 3) == 1);
    CHECK(StepDriverField(seatsB, 3) == 1);
    CHECK(StepDriverField(baselineSeats, 3) == 1);
    CHECK(memcmp(fieldA, baseline, sizeof(fieldA)) == 0);
    CHECK(seatsA[0].random == baselineSeats[0].random);
    PlayerCarRuntime inactive = fieldA[1];
    inactive.activeFlag = -1;
    PlayerCarRuntime inactiveSaved = inactive;
    seatsA[2].car = &inactive;
    seatsA[2].context = NULL;
    CHECK(StepDriverField(seatsA, 3) == 1);
    CHECK(memcmp(&inactive, &inactiveSaved, sizeof(inactive)) == 0);
    CHECK(seatsA[0].crashed == 0 && seatsA[2].crashed == 0);
    /* Three overlapping human cars exercise all three unordered pairs and
     * sparse seats up to the grid limit, with copied/restored state. */
    PlayerCarRuntime pileup[3], replayed[3];
    DriverSeat pileSeats[DRIVER_SEAT_LIMIT] = {0};
    DriverSeat replaySeats[DRIVER_SEAT_LIMIT] = {0};
    const int slots[3] = {0, 5, DRIVER_SEAT_LIMIT - 1};
    for (int i = 0; i < 3; i++) {
        InitDriver(&pileup[i], &spec, &engine, &start);
        pileSeats[slots[i]] = (DriverSeat){.car = &pileup[i], .context = &context,
            .hull = {hullPoints, corners}, .input = input, .random = (u32)i + 123};
    }
    for (int i = 0; i < 3; i++) {
        for (int j = i + 1; j < 3; j++) {
            const DriverHull hull = {hullPoints, corners};
            const DriverContact hit = FindDriverContact(&pileup[i], &hull,
                                                        &pileup[j], &hull, route.length);
            CHECK(hit.firstRegion != 0 && hit.secondRegion != 0);
        }
    }
    for (int tick = 0; tick < 20; tick++) {
        memcpy(replayed, pileup, sizeof(pileup));
        memcpy(replaySeats, pileSeats, sizeof(pileSeats));
        for (int i = 0; i < 3; i++) replaySeats[slots[i]].car = &replayed[i];
        CHECK(StepDriverField(pileSeats, DRIVER_SEAT_LIMIT));
        CHECK(StepDriverField(seatsB, 3)); /* another room interleaved */
        CHECK(StepDriverField(replaySeats, DRIVER_SEAT_LIMIT));
        CHECK(memcmp(pileup, replayed, sizeof(pileup)) == 0);
        for (int i = 0; i < 3; i++) {
            const int slot = slots[i];
            CHECK(pileSeats[slot].random == replaySeats[slot].random);
            CHECK(memcmp(&pileSeats[slot].step, &replaySeats[slot].step,
                         sizeof(DriverStep)) == 0);
            CHECK(pileSeats[slot].crashed == replaySeats[slot].crashed);
            if (tick == 0) {
                CHECK(pileSeats[slot].crashed == 1);
                CHECK(pileup[i].collisionFlag == 1);
            }
        }
    }
    return 0;
}
