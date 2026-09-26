#include "race_hud.h"
#include "game/race.h"
#include <stdio.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
static unsigned calls, warnings;
static s32 transmission;
static s32 position, times[6], count, visible, active, best, gear, speed, rpm, light, blend;
static TachometerLightingMode lighting;
static const CarTachometerSpec *dial;
void DrawRaceHudLabels(s32 mode) { if (mode == 1) ++calls; }
void DrawRacePosition(s32 value) { position = value; ++calls; }
void DrawLapTimes(const s32 *values, s32 laps, s32 shown, s32 current,
                  s32 fastest, s32 rivals) {
    (void)rivals;
    memcpy(times, values, (size_t)laps * sizeof(*values));
    count = laps; visible = shown; active = current; best = fastest; ++calls;
}
void DrawTachometer(const CarTachometerSpec *spec, s32 manual, s32 selectedGear, s32 selectedSpeed,
                    s32 selectedRpm, s32 shiftLight, TachometerLightingMode mode, s32 amount) {
    transmission = manual; dial = spec; gear = selectedGear; speed = selectedSpeed; rpm = selectedRpm;
    light = shiftLight; lighting = mode; blend = amount; ++calls;
}
void DrawWrongWayWarning(void) { ++warnings; }

int main(void) {
    RaceSim race = {.phase = SIM_RACING, .laps = 3, .elapsed = 6000};
    SimDriver *driver = &race.drivers[11];
    driver->status = SIM_DRIVING;
    driver->car.lap = 2;
    driver->lapTicks[0] = 5000;
    driver->lapStarted = 5000;
    driver->car.drive.manual = 1;
    driver->car.drive.gear = 4;
    driver->car.speed = 777;
    driver->wrongWayFrames = 10;
    race.drivers[0].status = SIM_DRIVER_FINISHED;
    race.drivers[0].place = 1;
    EngineSound engine = {.rpm = 6000, .jitter = 25, .shiftLight = 1};
    RaceSim unchanged = race;
    CHECK(DrawSimHud(&race, 11, &engine, TACHOMETER_LIGHTING_DARK, 45));
    CHECK(calls == 4 && warnings == 1 && position == 2);
    CHECK(count == 3 && visible == 2 && active == 1 && best == 100000);
    CHECK(times[0] == 100000 && times[1] == 20000 && times[2] == -1);
    CHECK(transmission == 1 && dial == &driver->spec.tachometer && gear == 4 && speed == 777);
    CHECK(rpm == 6025 && light == 1 && lighting == TACHOMETER_LIGHTING_DARK && blend == 45);
    CHECK(memcmp(&race, &unchanged, sizeof(race)) == 0);
    driver->status = SIM_DRIVER_FINISHED;
    driver->car.activeFlag = -1;
    driver->car.lap = 4;
    driver->lapTicks[1] = 4000;
    driver->lapTicks[2] = 3000;
    driver->place = 1;
    CHECK(DrawSimHud(&race, 11, &engine, TACHOMETER_LIGHTING_NORMAL, 0));
    CHECK(position == 1 && best == 60000 && warnings == 1);
driver->wrongWayFrames = 9;
driver->status = SIM_DRIVING; driver->car.activeFlag = 0;
CHECK(DrawSimHud(&race, 11, &engine, TACHOMETER_LIGHTING_NORMAL, 0));
CHECK(warnings == 1);
driver->status = SIM_DRIVER_FINISHED; driver->car.activeFlag = -1;
    const unsigned before = calls;
    CHECK(!DrawSimHud(NULL, 11, &engine, TACHOMETER_LIGHTING_NORMAL, 0));
    CHECK(!DrawSimHud(&race, -1, &engine, TACHOMETER_LIGHTING_NORMAL, 0));
    CHECK(!DrawSimHud(&race, 12, &engine, TACHOMETER_LIGHTING_NORMAL, 0));
    CHECK(!DrawSimHud(&race, 11, NULL, TACHOMETER_LIGHTING_NORMAL, 0));
    driver->status = SIM_RETIRED;
    CHECK(!DrawSimHud(&race, 11, &engine, TACHOMETER_LIGHTING_NORMAL, 0));
    driver->status = SIM_DRIVING;
    CHECK(!DrawSimHud(&race, 11, &engine, TACHOMETER_LIGHTING_NORMAL, 0));
    driver->car.activeFlag = 0; driver->rival = 1;
    CHECK(!DrawSimHud(&race, 11, &engine, TACHOMETER_LIGHTING_NORMAL, 0));
driver->rival = 0; driver->car.lap = INT16_MIN;
CHECK(!DrawSimHud(&race, 11, &engine, TACHOMETER_LIGHTING_NORMAL, 0));
    CHECK(calls == before);
    return 0;
}
