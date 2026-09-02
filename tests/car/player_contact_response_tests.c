#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/track_internal.h"

#include <stdio.h>
#include <string.h>

GameTrackPoint *g_TrackPoints;
s32 g_TrackPointCount;
s32 g_MirrorMode;
s16 g_RacePhase;
s32 g_ShiftTargetRpm;

static GameTrackPoint s_points[2];
static s32 s_slip;
static int s_bodyKickCalls;
static int s_soundCalls;
static s32 s_soundCue;
static int s_failures;

s32 GetAngleDistance(s32 from, s32 to) {
    (void)from;
    (void)to;
    return s_slip;
}

s32 rsin(s32 angle) {
    (void)angle;
    return 4096;
}

void UpdateCarBodyKick(GameCarRuntime *car) {
    (void)car;
    s_bodyKickCalls++;
}

void PlaySoundCue(s32 cue) {
    s_soundCue = cue;
    s_soundCalls++;
}

static void Reset(PlayerCarRuntime *car) {
    memset(car, 0, sizeof(*car));
    memset(s_points, 0, sizeof(s_points));
    g_TrackPoints = s_points;
    g_TrackPointCount = 2;
    g_MirrorMode = 0;
    g_RacePhase = 2;
    g_ShiftTargetRpm = 2000;
    s_slip = 500;
    s_bodyKickCalls = 0;
    s_soundCalls = 0;
    s_soundCue = 0;
    car->drive.launchEnergy = 10000;
    car->drive.drivetrainTorque = 10000;
    car->drive.engineLoad = 1000;
    car->motionTimer = 15;
}

#define CHECK(condition) do {                                                \
    if (!(condition)) {                                                      \
        printf("FAIL line %d: %s\n", __LINE__, #condition);                 \
        s_failures++;                                                        \
    }                                                                        \
} while (0)

static void CheckSkidCue(s32 skid, s32 slip, s32 mirror, s32 initialSpeed,
                         s32 expectedSpeed, s32 expectedCue) {
    PlayerCarRuntime car;

    Reset(&car);
    car.speed = initialSpeed;
    s_slip = slip;
    g_MirrorMode = mirror;
    ApplyPlayerContactResponse(&car, skid, 0);
    CHECK(s_soundCalls == 1 && s_soundCue == expectedCue);
    CHECK(car.drive.launchEnergy == 5000);
    CHECK(car.drive.drivetrainTorque == 6500);
    CHECK(car.speed == expectedSpeed);
    CHECK(car.drive.engineLoad == 650);
    CHECK(g_ShiftTargetRpm == 1300);
}

int main(void) {
    PlayerCarRuntime car;

    Reset(&car);
    car.y = 100;
    car.drive.standingStartBounceY = 7;
    ApplyPlayerContactResponse(&car, 0, 0);
    CHECK(car.y == 107 && s_bodyKickCalls == 1 && s_soundCalls == 0);

    Reset(&car);
    car.speed = 80;
    ApplyPlayerContactResponse(&car, 0, 1);
    CHECK(car.drive.launchEnergy == 9000);
    CHECK(car.speed == 80 && car.drive.drivetrainTorque == 10000);

    Reset(&car);
    car.speed = 81;
    ApplyPlayerContactResponse(&car, 0, 1);
    CHECK(car.drive.launchEnergy == 9000);
    CHECK(car.speed == 78 && car.drive.drivetrainTorque == 9800);
    CHECK(car.drive.engineLoad == 950 && g_ShiftTargetRpm == 1900);

    CheckSkidCue(1, 800, 0, 100, 47, 0xA);
    CheckSkidCue(3, 800, 0, 200, 94, 0xD);
    CheckSkidCue(1, 500, 0, 100, 47, 0xB);
    CheckSkidCue(1, 500, 1, 100, 47, 0xC);
    CheckSkidCue(2, 500, 0, 100, 47, 0xC);
    CheckSkidCue(2, 500, 1, 100, 47, 0xB);

    Reset(&car);
    car.speed = 100;
    car.motionTimer = 14;
    ApplyPlayerContactResponse(&car, 1, 0);
    CHECK(s_soundCalls == 0);

    Reset(&car);
    car.speed = 100;
    g_RacePhase = 3;
    ApplyPlayerContactResponse(&car, 1, 0);
    CHECK(s_soundCalls == 0);

    if (s_failures != 0) {
        printf("%d player contact response checks failed\n", s_failures);
        return 1;
    }
    puts("player skid and collision responses preserve their thresholds");
    return 0;
}
