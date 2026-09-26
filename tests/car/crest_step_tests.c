#include "game/car_motion_internal.h"
#include "game/track.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

int main(void) {
    TrackEventData firstEvents = {0};
    TrackEventData secondEvents = {0};
    firstEvents.crestEvents[0][0].progress = 100;
    firstEvents.crestEvents[0][0].motionValue = 10;
    firstEvents.crestEvents[0][1].motionValue = -1;
    firstEvents.crestEvents[1][0].motionValue = -1;
    secondEvents.crestEvents[0][0].motionValue = -1;
    secondEvents.crestEvents[1][0].progress = 100;
    secondEvents.crestEvents[1][0].motionValue = -20;
    secondEvents.crestEvents[1][1].motionValue = -1;
    GameCarRuntime initial = {0};
    initial.speed = 960;
    initial.previousTrackProgress = 90;
    initial.trackProgress = 100;
    initial.bodyPitch = 10;
    initial.bodyRoll = -4;
    initial.y = 500;
    CHECK(FindCarCrest(&initial, &firstEvents, 1000, 0) == 10);
    GameCarRuntime first = initial;
    StepCarCrestHop(&first, &firstEvents, 1000, 0);
    CHECK(first.verticalMotionState == CAR_VERTICAL_RISING);
    CHECK(first.verticalMotionRate == -2 && first.verticalMotionTimer == 0);
    CHECK(first.verticalTargetY == 500 && first.verticalPitch == 10);

    GameCarRuntime second = initial;
    second.facingBackwards = 2;
    StepCarCrestHop(&second, &secondEvents, 2000, 0);
    CHECK(second.verticalMotionState == CAR_VERTICAL_AT_CREST);
    CHECK(second.verticalMotionRate == 20 && first.verticalMotionRate == -2);
    GameCarRuntime restored = initial;
    StepCarCrestHop(&restored, &firstEvents, 1000, 0);
    CHECK(memcmp(&first, &restored, sizeof(first)) == 0);
    first.verticalMotionTimer = 6;
    StepCarCrestHop(&first, NULL, 0, 0);
    CHECK(first.verticalPitch == 16 && first.verticalRoll == -3);
    CHECK(first.bodyPitch == 16 && first.bodyRoll == -3);
    first.verticalPitch = 300;
    StepCarCrestHop(&first, NULL, 0, 0);
    CHECK(first.verticalPitch == 300);

    initial.trackProgress = 900;
    initial.previousTrackProgress = 910;
    CHECK(FindCarCrest(&initial, &firstEvents, 1000, 1) == 10);
    CHECK(FindCarCrest(&initial, &firstEvents, 2000, 1) == 0);
    initial.speed = 799;
    CHECK(FindCarCrest(&initial, &firstEvents, 1000, 1) == 0);
    initial.speed = 800;
    CHECK(FindCarCrest(&initial, &firstEvents, 1000, 1) == 10);
    CHECK(FindCarCrest(&initial, NULL, 1000, 1) == 0);
    CHECK(FindCarCrest(NULL, &firstEvents, 1000, 1) == 0);
    CHECK(FindCarCrest(&initial, &firstEvents, 0, 1) == 0);
    initial.previousTrackProgress = 0;
    initial.trackProgress = 4096;
    CHECK(FindCarCrest(&initial, &firstEvents, 10000, 0) == 0);
    PlayerCarRuntime human = {0};
    GameCarSpec spec = {0};
    human.speed = 960;
    human.previousTrackProgress = 90;
    human.trackProgress = 100;
    human.y = 500;
    StepCarCrestHop(AsRivalCar(&human), &firstEvents, 1000, 0);
    CHECK(StepPlayerJump(&human, &spec, 500) == 0);
    CHECK(human.y == 498 && human.verticalMotionTimer == 1);
    CHECK(human.verticalMotionState == CAR_VERTICAL_RISING);
    puts("crest step tests passed");

    return 0;
}
