#include "race_hud.h"
#include "game/race.h"
#include "game/integer.h"

int DrawSimHud(const RaceSim *race, s32 seat, const EngineSound *engine,
                TachometerLightingMode lighting, s32 blendAmount) {
    if (!race || !engine || (u32)seat >= DRIVER_SEAT_LIMIT ||
        race->laps < 1 || race->laps > PLAYER_LAP_TIME_CAPACITY) return 0;
    const SimDriver *driver = &race->drivers[seat];
    if (driver->car.lap < 0 || driver->car.lap > race->laps + 1 || driver->rival ||
        (driver->status != SIM_DRIVING && driver->status != SIM_DRIVER_FINISHED) ||
        (driver->status == SIM_DRIVING && driver->car.activeFlag == -1)) return 0;
    s32 times[PLAYER_LAP_TIME_CAPACITY];
    s32 best = -1;
    for (s32 lap = 0; lap < race->laps; ++lap) {
        times[lap] = RaceLapTime(race, seat, lap);
        /* The current lap must not become a personal best before crossing. */
        if (driver->car.lap > lap + 1 && times[lap] >= 0 &&
            (best < 0 || times[lap] < best)) best = times[lap];
    }
    DrawRaceHudLabels(1);
    DrawRacePosition(RacePosition(race, seat));
    DrawLapTimes(times, race->laps, driver->car.lap,
                 driver->car.lap - 1, best, 1);
    DrawTachometer(&driver->spec.tachometer, driver->car.drive.manual, driver->car.drive.gear,
                  driver->car.speed, WrapSigned32((int64_t)engine->rpm + engine->jitter),
                  engine->shiftLight, lighting, blendAmount);
    if (driver->status == SIM_DRIVING && driver->wrongWayFrames >= WRONG_WAY_WARNING_FRAMES)
        DrawWrongWayWarning();
    return 1;
}
