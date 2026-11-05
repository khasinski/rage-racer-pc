#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/track.h"
#include "rage/speed_display.h"
#include "render/speed_units_poc.h"
#include <stdio.h>
#include <string.h>

GameCarRuntime g_Cars[RACE_CAR_SLOT_COUNT];
PlayerCarRuntime g_PlayerCar;
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
    {
        static uint8_t rgba[384 * 376 * 4], original[sizeof(rgba)];
        uint16_t face[24 * 94], before[24 * 94], palette[16] = {0};
        s_region = "PAL";
        for (int car = 0; car <= 3; car += 3) {
            int y0 = car == 0 ? 6 : 7, width = car == 0 ? 15 : 26;
            for (unsigned i = 0; i < sizeof(face)/sizeof(face[0]); ++i) face[i] = 0x1111;
            palette[1] = 0x0421; palette[2] = 0x7fff;
            face[y0 * 24 + 17] = 0x2222;
            memcpy(before, face, sizeof(face));
            RageSpeedUnitsConfigure(car, "PAL", "auto");
            RageSpeedUnitsPatchIndexed(face, palette);
            CHECK(!memcmp(before, face, sizeof(face)));
            RageSpeedUnitsConfigure(car, "PAL", "mph");
            CHECK(SpeedDisplayValue(1168) == 100);
            CHECK(SpeedDisplayValue(-1) == 0);
            CHECK(SpeedDisplayValue(1000000) == 999);
            RageSpeedUnitsPatchIndexed(face, palette);
            CHECK(memcmp(before, face, sizeof(face)));
            for (int y = 0; y < 94; ++y) for (int x = 0; x < 96; ++x) {
                unsigned shift = (unsigned)(x & 3) * 4u;
                if (x >= 65 && x < 65 + width && y >= y0 && y < y0 + 7) continue;
                CHECK(((face[y*24+x/4] ^ before[y*24+x/4]) & (15u << shift)) == 0);
            }
        }
        /* Scaled HD patch: preserve every pixel outside the label and alpha. */
        memset(rgba, 17, sizeof(rgba));
        memset(rgba + (7 * 4 * 384 + 68 * 4) * 4, 200, 4);
        memcpy(original, rgba, sizeof(rgba));
        RageSpeedUnitsPatchRGBA("ui.hud.car.3.page.10.clut.13224.region.0.112.96.94", rgba, 384, 376);
        CHECK(memcmp(original, rgba, sizeof(rgba)));
        for (int y = 0; y < 376; ++y) for (int x = 0; x < 384; ++x) {
            int i = (y * 384 + x) * 4;
            CHECK(rgba[i+3] == original[i+3]);
            if (x >= 65*4 && x < 91*4 && y >= 7*4 && y < 14*4) continue;
            CHECK(!memcmp(rgba+i, original+i, 4));
        }
        memcpy(rgba, original, sizeof(rgba));
        RageSpeedUnitsPatchRGBA("ui.hud.car.4.page.10.clut.13224.region.0.112.96.94", rgba, 384, 376);
        CHECK(!memcmp(original, rgba, sizeof(rgba)));
        RageSpeedUnitsPatchRGBA("ui.hud.car.3.page.10.clut.13224.region.0.112.96.94", rgba, 383, 376);
        CHECK(!memcmp(original, rgba, sizeof(rgba)));
        RageSpeedUnitsConfigure(3, "PAL", "kmh");
        CHECK(SpeedDisplayValue(1168) == 160);
        RageSpeedUnitsConfigure(4, "PAL", "mph");
        CHECK(RageSpeedUnits() == RAGE_UNITS_AUTO);
        CHECK(SpeedDisplayValue(1168) == 160);
        RageSpeedUnitsConfigure(3, "NTSC-U", "kmh");
        s_region = "NTSC-U";
        CHECK(RageSpeedUnits() == RAGE_UNITS_AUTO);
        CHECK(SpeedDisplayValue(1168) == 100);
        RageSpeedUnitsConfigure(3, "PAL", "auto");
    }
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
    /* A negative threshold keeps the pursuit active until the rival has
     * pulled ahead by the requested distance. */
    {
        GameCarRuntime *car = &g_Cars[0];
        int normal, boosted;
        g_CpuChaseMode = 0;
        car->trackProgress = 64 * 16;
        car->speedKeyIndex = 0;
        UpdateCarAiTargetSpeed(car, 0);
        normal = car->accelerationLimit;
        g_CpuChaseMode = 1;
        g_CpuChaseRivalCar = 0;
        g_CpuChaseNearDistance = -10;
        g_CpuChaseFarDistance = 50;
        g_CpuChaseSpeedPercent = 150;
        g_PlayerCar.progressA = 20 * 32;
        car->progressA = 0;
        car->progressB = 0;
        UpdateCarAiTargetSpeed(car, 0);
        boosted = car->accelerationLimit;
        CHECK(boosted > normal);
        car->progressA = g_PlayerCar.progressA + 11 * 32;
        UpdateCarAiTargetSpeed(car, 0);
        CHECK(car->accelerationLimit == normal);
    }
    puts("regional speed units: rival state identical for all 300 ticks");
    return 0;
}
