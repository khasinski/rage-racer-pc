#include "game/race_grid.h"

int InitRaceGrid(RaceSim *race, const RaceData *archive, const TrackData *track,
                  const RaceEntrant entrants[DRIVER_SEAT_LIMIT],
                  const RageCarCatalog *catalog, s32 laps, int reverse) {
    if (!race || !archive || !track || !track->events || !entrants ||
        (catalog && !CarCatalogValidate(catalog, NULL, 0))) return 0;
    RaceSim candidate;
    if (!InitRaceSim(&candidate, &track->route, track->events, laps, reverse)) return 0;
    const DriverHull hull = {g_PlayerHullPoints, g_OpponentHullCorners};
    const LaunchSpeedThreshold launch = {960, 320};
    int count = 0;
    u32 usedGrid = 0;
    for (s32 seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) {
        const RaceEntrant *entrant = &entrants[seat];
        if (entrant->kind == RACE_SEAT_EMPTY) continue;
        if ((u32)entrant->grid >= DRIVER_SEAT_LIMIT ||
            (usedGrid & (1u << entrant->grid))) return 0;
        usedGrid |= 1u << entrant->grid;
        const TrackRivalStart *position = entrant->kind == RACE_SEAT_HUMAN && entrant->hasStart
            ? &entrant->start : &track->events->rivalStarts[reverse][entrant->grid];
        if (entrant->kind == RACE_SEAT_HUMAN) {
            GameCarSpec spec;
            if (!ReadRaceCar(archive, entrant->model, &spec)) return 0;
            if (catalog) {
                for (size_t i = 0; i < catalog->count; ++i) {
                    const RageCarCatalogEntry *entry = &catalog->entries[i];
                    if (CarCatalogVariant(entry->modelIndex, entry->grade) == entrant->model) {
                        ApplyCarSpec(entry, &spec);
                        break;
                    }
                }
            }
            if (!AddRaceDriver(&candidate, seat, &spec, &hull, g_CarCornerOffsets,
                               &launch, position, track->events->trackWalkStart,
                               entrant->manual, 0x17, entrant->seed)) return 0;
            candidate.drivers[seat].variant = entrant->model;
        } else if (entrant->kind == RACE_SEAT_AI) {
            if ((u32)entrant->model >= RACE_CAR_SLOT_COUNT ||
                !AddRaceRival(&candidate, seat, entrant->rivalSlot, &hull,
                              g_CarCollisionCorners, position, track->events->trackWalkStart,
                              (u16)entrant->model)) return 0;
            candidate.drivers[seat].random = entrant->seed;
        } else return 0;
        ++count;
    }
    if (!count) return 0;
    *race = candidate;
    return 1;
}
