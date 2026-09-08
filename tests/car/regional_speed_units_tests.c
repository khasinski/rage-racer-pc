#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/track.h"
#include "rage/speed_display.h"
#include <stdio.h>
#include <string.h>

GameCarRuntime g_Cars[RACE_CAR_SLOT_COUNT];
static TrackEventData s_events;
const TrackEventData *g_TrackEventData = &s_events;
s32 g_RaceSeries;
s32 g_TrackLength = 0x8000;
static const char *s_region;
const char *HostDiscRegion(void) { return s_region; }
s32 GetAngleDelta(s32 from, s32 to) { return to - from; }
void UpdateCarSlideAngle(GameCarRuntime *car, s32 pitch) {
    (void)car; (void)pitch;
}
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; } } while (0)

int main(void) {
    static GameCarRuntime reference[300];
    const char *regions[] = {"PAL", "NTSC-U", "NTSC-J"};
    RaceGridSlot grid[RACE_CAR_SLOT_COUNT] = {0};
    s_events.rivalAiConfigs[0][0].speed = 160;
    s_events.rivalAiConfigs[0][0].accelerationStep = 5;
    s_events.aiSpeedKeys[0][0].progress = 32;
    s_events.aiSpeedKeys[0][1].progress = 256;
    s_events.aiSpeedKeys[0][0].slotTargetSpeeds[0] = 160;
    s_events.aiSpeedKeys[0][1].slotTargetSpeeds[0] = 240;
    for (unsigned r = 0; r < sizeof(regions)/sizeof(regions[0]); ++r) {
        s_region = regions[r];
        CHECK(SpeedDisplayValue(1168) == (r == 1 ? 100 : 160));
        memset(g_Cars, 0, sizeof(g_Cars));
        for (int c = 1; c < RACE_CAR_SLOT_COUNT; ++c) g_Cars[c].activeFlag = -1;
        GameCarRuntime *car = &g_Cars[0];
        InitRivalCarAi(car, 0, grid);
        CHECK(car->targetSpeed == 1168);
        for (int tick = 0; tick < 300; ++tick) {
            car->trackProgress = (32 + tick % 225) * 16;
            UpdateCarAiTargetSpeed(car, 0);
            AccelerateRaceRivals();
            GameCarRuntime before = *car;
            (void)SpeedDisplayValue(car->speed);
            CHECK(memcmp(car, &before, sizeof(before)) == 0);
            if (r == 0) reference[tick] = *car;
            else CHECK(memcmp(car, &reference[tick], sizeof(*car)) == 0);
        }
    }
    puts("regional speed units: rival state identical for all 300 ticks");
    return 0;
}
