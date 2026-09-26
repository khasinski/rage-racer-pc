#include "game/car_track_internal.h"
#include "game/car_motion_internal.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

int main(void) {
    const GameTrackPoint points[] = {
        {.x = 0, .z = 0, .segmentLength = 1000,
         .leftHalfWidth = 100, .rightHalfWidth = 100},
        {.x = 1000, .z = 0, .segmentLength = 1000,
         .leftHalfWidth = 100, .rightHalfWidth = 100},
        {.x = 2000, .z = 0, .segmentLength = 1000,
         .leftHalfWidth = 100, .rightHalfWidth = 100},
    };
    const GameTrackPoint otherPoints[] = {
        {.x = 0, .z = 0, .y = 50, .segmentLength = 500,
         .leftHalfWidth = 300, .rightHalfWidth = 300},
        {.x = 500, .z = 0, .y = 50, .segmentLength = 500,
         .leftHalfWidth = 300, .rightHalfWidth = 300},
    };
    const TrackRoute firstRoute = {.points = points, .count = 3, .length = 3000};
    const TrackRoute secondRoute = {.points = otherPoints, .count = 2, .length = 1000};
    const CarTrackLimits limits = {
        .leftContact = CAR_TRACK_CONTACT_REAR_RIGHT,
        .rightContact = CAR_TRACK_CONTACT_REAR_RIGHT,
    };
    GameCarRuntime initial = {0};
    initial.x = 200;
    initial.z = 150;
    initial.trackProgress = 123;
    GameCarRuntime first = initial;
    CHECK(StepCarTrackState(&first, &firstRoute, 0, &limits, 0, 1) ==
          CAR_TRACK_CONTACT_REAR_RIGHT);
    CHECK(first.z == 100 && first.y == 0);
    CHECK(first.motionActive == 1 && first.velocityZ == 25);
    CHECK(first.previousTrackProgress == 123 && first.trackProgress == 800);

    GameCarRuntime second = initial;
    CHECK(StepCarTrackState(&second, &secondRoute, 0, &limits, 1, 1) == 0);
    CHECK(second.z == 150 && second.y == 50 && second.motionActive == 0);
    CHECK(second.trackProgress == 200);
    GameCarRuntime repeated = initial;
    StepCarTrackState(&repeated, &firstRoute, 0, &limits, 0, 1);
    CHECK(memcmp(&first, &repeated, sizeof(first)) == 0);

    /* Actual segment search feeds progress without a fake search callback. */
    GameCarRuntime moving = {0};
    moving.x = 1200;
    moving.z = 0;
    s32 segment = FindCarTrackSegment(&moving, &firstRoute, 0);
    CHECK(segment == 1);
    MoveCarTrackProgress(&moving, &firstRoute, segment, 1);
    CHECK(moving.trackPointIndex == 1 && moving.progressA == 1000);
    StepCarTrackState(&moving, &firstRoute, segment, &limits, 1, 1);
    CHECK(moving.trackProgress == 1200);
    /* Every human gets a response; AI callers may request clamping only. */
    repeated = initial;
    StepCarTrackState(&repeated, &firstRoute, 0, &limits, 0, 0);
    CHECK(repeated.z == 100 && repeated.motionActive == 0);
    repeated = initial;
    repeated.z = -150;
    StepCarTrackState(&repeated, &firstRoute, -3, &limits, 0, 1);
    CHECK(repeated.z == -100 && repeated.velocityZ == -25);
    CHECK(first.velocityZ == 25);
    ApplyCarKnockback(&repeated);
    CHECK(repeated.z == -75 && first.z == 100);

    repeated = initial;
    CHECK(StepCarTrackState(&repeated, NULL, 0, &limits, 0, 1) == 0);
    CHECK(memcmp(&initial, &repeated, sizeof(initial)) == 0);
    CHECK(StepCarTrackState(NULL, &firstRoute, 0, &limits, 0, 1) == 0);
    puts("track contact step tests passed");
    return 0;
}
