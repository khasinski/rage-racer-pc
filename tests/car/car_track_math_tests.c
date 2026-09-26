#include "game/car_track_internal.h"

#include <limits.h>
#include <stdio.h>

#define CHECK_EQ(actual, expected) do {                                        \
    if ((actual) != (expected)) {                                               \
        fprintf(stderr, "line %d: %s = %d, expected %d\n", __LINE__, #actual, \
                (int)(actual), (int)(expected));                                \
        return 1;                                                               \
    }                                                                           \
} while (0)

int main(void) {
    CHECK_EQ(InterpolateCarTrackValue(10, 30, 0, 20), 10);
    CHECK_EQ(InterpolateCarTrackValue(10, 30, 10, 20), 20);
    CHECK_EQ(InterpolateCarTrackValue(10, 30, 20, 20), 30);
    CHECK_EQ(InterpolateCarTrackValue(10, 30, 8, 0), 10);
    CHECK_EQ(InterpolateCarTrackValue(10, 30, 8, -1), 10);
    CHECK_EQ(InterpolateCarTrackValue(INT_MAX, INT_MAX, 1, 2), -1);
    CHECK_EQ(InterpolateCarTrackValue(1, 1, INT_MIN, 1), 1);

    CHECK_EQ(CarTrackFixed12ToInteger(0x1FFF), 1);
    CHECK_EQ(CarTrackFixed12ToInteger(-0x1FFF), -1);
    CHECK_EQ(CarTrackFixed12ToInteger(-1), 0);

    CHECK_EQ(ProjectCarTrackAxis(0x4000), 1);
    CHECK_EQ(ProjectCarTrackAxis(-0x4000), -1);
    CHECK_EQ(ProjectCarTrackAxis(-0x1000), -1);
    CHECK_EQ(ProjectCarTrackAxis(-0x0FFF), 0);

    CHECK_EQ(ClampCarTrackAlongSegment(-1, 100), 0);
    CHECK_EQ(ClampCarTrackAlongSegment(0, 100), 0);
    CHECK_EQ(ClampCarTrackAlongSegment(40, 100), 40);
    CHECK_EQ(ClampCarTrackAlongSegment(100, 100), 100);
    CHECK_EQ(ClampCarTrackAlongSegment(101, 100), 100);

    CHECK_EQ(InterpolateCarTrackHeading(100, 300, 0, 20), 100);
    CHECK_EQ(InterpolateCarTrackHeading(100, 300, 10, 20), 200);
    CHECK_EQ(InterpolateCarTrackHeading(100, 300, 20, 20), 300);
    CHECK_EQ(InterpolateCarTrackHeading(0xF00, 0x100, 10, 20), 0);
    CHECK_EQ(InterpolateCarTrackHeading(123, 456, 8, 0), 123);
    CHECK_EQ(InterpolateCarTrackHeading(123, 456, 8, -1), 123);
    CHECK_EQ(InterpolateCarTrackHeading(INT16_MAX, INT16_MAX,
                                        INT_MAX, 1), INT16_MAX);
    CHECK_EQ(InterpolateCarTrackHeading(1, 1, INT_MIN, 1), 1);

GameCarRuntime first = {0};
GameCarRuntime second = {0};
first.progressA = 900;
first.progressB = 400;
first.trackProgress = 700;
second.progressA = -400;
second.progressB = 100;
second.trackProgress = 500;

UpdateCarLapProgressState(&first, 1000, 0);
UpdateCarLapProgressState(&second, 2000, 1);
CHECK_EQ(first.previousTrackProgress, 700);
CHECK_EQ(first.trackProgress, 300);
CHECK_EQ(first.trackSection, 1);
CHECK_EQ(second.previousTrackProgress, 500);
CHECK_EQ(second.trackProgress, 1700);
CHECK_EQ(second.trackSection, 1);

/* Interleaving a different course/direction must not change this car. */
UpdateCarLapProgressState(&first, 1000, 1);
CHECK_EQ(first.previousTrackProgress, 300);
CHECK_EQ(first.trackProgress, 300);
CHECK_EQ(first.trackSection, 2);
UpdateCarLapProgressState(&first, 0, 0);
UpdateCarLapProgressState(&first, -1, 0);
CHECK_EQ(first.previousTrackProgress, 300);
CHECK_EQ(first.trackProgress, 300);
CHECK_EQ(first.trackSection, 2);

first.progressA = INT_MAX;
first.progressB = 1;
UpdateCarLapProgressState(&first, 1000, 0);
CHECK_EQ(first.trackProgress, 352);
first.progressA = 2000;
first.progressB = 0;
UpdateCarLapProgressState(&first, 1000, 1);
CHECK_EQ(first.trackProgress, 0);
CHECK_EQ(first.trackSection, 3);
puts("car track math tests passed");

    return 0;
}
