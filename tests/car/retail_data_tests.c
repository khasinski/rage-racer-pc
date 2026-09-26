#include "game/track_data.h"
#include "game/race_data.h"
#include "game/race_grid.h"
#include "game/race_sim.h"
#include "game/car_catalog.h"
#include "game/rival.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
/* Exercise the production grid constructor on the imported track, separately
 * from the longer handling/finish sweeps below. */
static int CheckRetailGrid(const RaceData *archive, const TrackData *track) {
    for (int reverse = 0; reverse < 2; ++reverse) {
        for (int mixed = 0; mixed < 2; ++mixed) {
            RaceEntrant entrants[DRIVER_SEAT_LIMIT] = {0};
            for (s32 seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) {
                if (mixed && seat > 0 && seat < DRIVER_SEAT_LIMIT - 1) {
                    if (track->events->rivalStarts[reverse][seat].activeFlag == -1) continue;
                    entrants[seat] = (RaceEntrant){.kind = RACE_SEAT_AI, .grid = seat,
                        .model = seat - 1, .rivalSlot = seat - 1, .seed = (u32)seat + 1};
                } else {
                    entrants[seat] = (RaceEntrant){.kind = RACE_SEAT_HUMAN, .grid = seat,
                        .model = seat, .manual = 1, .seed = (u32)seat + 1};
                }
            }
            RaceSim race;
            if (!InitRaceGrid(&race, archive, track, entrants, NULL, 1, reverse)) return 0;
            for (s32 seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) {
                const SimDriver *driver = &race.drivers[seat];
                if (entrants[seat].kind == RACE_SEAT_EMPTY) {
                    if (driver->status != SIM_EMPTY) return 0;
                    continue;
                }
                if (driver->status != SIM_DRIVING || driver->random != (u32)seat + 1) return 0;
                if (entrants[seat].kind == RACE_SEAT_HUMAN) {
                    GameCarSpec spec;
                    if (!ReadRaceCar(archive, seat, &spec) ||
                        driver->variant != seat || driver->spec.revLimit != spec.revLimit || driver->car.drive.manual != 1) return 0;
                }
            }
            if (!StartRaceSim(&race, 0)) return 0;
            for (int tick = 0; tick < 50; ++tick) {
                const DriverInput input = {.throttle = 256};
                for (s32 seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat)
                    if (entrants[seat].kind == RACE_SEAT_HUMAN && !SetRaceInput(&race, seat, &input)) return 0;
                if (!StepRaceSim(&race)) return 0;
            }
        }
    }
    return 1;
}

/* laps == 0 runs the short input sweep; otherwise drive that many laps. */
static int StepRetailRace(const TrackData *track, const GameCarSpec *spec, int reverse, int laps, int rivals, s16 manual, s16 model, int humans) {
    const DriverHull hull = {g_PlayerHullPoints, g_OpponentHullCorners};
    const LaunchSpeedThreshold threshold = {960, 320};
    RaceSim race, restored;
    if (!InitRaceSim(&race, &track->route, track->events, laps ? laps : 1, reverse)) return 0;
    for (int seat = 0; seat < humans; seat++) {
        if (!AddRaceDriver(&race, seat, spec, &hull, g_CarCornerOffsets, &threshold,
            &track->events->rivalStarts[reverse][seat], track->events->trackWalkStart,
            manual, model, (u32)seat + 1)) return 0;
    }
    s32 entrants = humans;
    if (rivals) {
        for (s32 seat = humans; seat < DRIVER_SEAT_LIMIT; seat++) {
            const TrackRivalStart *position = &track->events->rivalStarts[reverse][seat];
            if (position->activeFlag == -1) continue;
            if (!AddRaceRival(&race, seat, seat - 1, &hull, g_CarCollisionCorners, position,
                track->events->trackWalkStart, (u16)(seat - 1))) {
                fprintf(stderr, "AI setup failed: seat=%d point=%d active=%d\n",
                    seat, position->trackPointIndex, position->activeFlag); return 0;
            }
            entrants++;
        }
    }
    int shifted[DRIVER_SEAT_LIMIT] = {0}, moved[DRIVER_SEAT_LIMIT] = {0};
    struct { s32 x, z; } starts[DRIVER_SEAT_LIMIT];
    for (int seat = 0; seat < humans; seat++) {
        starts[seat].x = race.drivers[seat].car.x;
        starts[seat].z = race.drivers[seat].car.z;
    }
    if (!StartRaceSim(&race, 50)) return 0;
    for (int tick = 0; tick < (laps ? 100000 : 550) && race.phase != SIM_FINISHED; tick++) {
        DriverInput input = {.steering.mode = STEERING_ANALOG,
            .steering.angle = tick % 100 < 50 ? 256 : -256, .throttle = 256};
        for (int seat = 0; seat < humans; seat++) {
            if (race.drivers[seat].status != SIM_DRIVING) continue;
            if (!manual || laps || humans == DRIVER_SEAT_LIMIT) {
                PlayerCarRuntime guide = race.drivers[seat].car;
                const SteeringInput automatic = {.mode = STEERING_AUTOMATIC};
                UpdateCarSteering(&guide, &automatic);
                input.steering.angle = (s16)guide.drive.steerPos;
                if (laps) {
                    input.throttle = guide.speed < 400 ? 256 : 0;
                    input.brake = guide.speed > 450 ? 128 : 0;
                }
            }
            if (!SetRaceInput(&race, seat, &input)) return 0;

        }
        const RaceSim previous = race;
        restored = race;
        if (!StepRaceSim(&race) || !StepRaceSim(&restored)) {
            fprintf(stderr, "simulation step rejected: tick=%d\n", tick); return 0;
        }
        if (memcmp(&race, &restored, sizeof(race)) != 0) {
            const unsigned char *a = (const unsigned char *)&race;
            const unsigned char *b = (const unsigned char *)&restored;
            for (size_t offset = 0; offset < sizeof race; offset++) {
                if (a[offset] != b[offset]) {
                    fprintf(stderr, "restoration differs: tick=%d byte=%zu values=%u,%u\n",
                        tick, offset, a[offset], b[offset]); break;
                }
            }
            return 0;
        }
        for (int seat = 0; seat < humans; seat++) {
            const PlayerCarRuntime *car = &race.drivers[seat].car;
            if (car->drive.gear > 1) shifted[seat] = 1;
            if (car->x != starts[seat].x || car->z != starts[seat].z) moved[seat] = 1;
        }
        for (int seat = 0; seat < DRIVER_SEAT_LIMIT; seat++) {
            if (race.drivers[seat].status == SIM_EMPTY) continue;
            if (previous.drivers[seat].status == SIM_DRIVER_FINISHED &&
                memcmp(&race.drivers[seat], &previous.drivers[seat], sizeof(SimDriver)) != 0) return 0;
            if ((u32)race.drivers[seat].car.trackPointIndex >= (u32)track->route.count) return 0;
        }
    }
    for (int seat = 0; seat < humans; seat++) {
        if (!manual && !shifted[seat]) {
            fprintf(stderr, "automatic did not upshift: model=%d reverse=%d seat=%d\n",
                    model, reverse, seat); return 0;
        }
    }
    if (laps) {
        if (race.phase != SIM_FINISHED || race.finishCount != entrants) {
            fprintf(stderr, "lap incomplete: reverse=%d elapsed=%u laps=%d,%d\n",
                reverse, race.elapsed, race.drivers[0].car.lap, race.drivers[1].car.lap);
            return 0;
        }
        const SimDriver *first = &race.drivers[0], *second = &race.drivers[1];
        if (first->status != SIM_DRIVER_FINISHED || second->status != SIM_DRIVER_FINISHED ||
            first->place == second->place ||
            first->lapTicks[0] == 0 || second->lapTicks[0] == 0 ||
            first->finishTick > race.elapsed || second->finishTick > race.elapsed ||
            (first->place == 1 && first->finishTick > second->finishTick) ||
            (second->place == 1 && second->finishTick > first->finishTick)) return 0;
        u32 places = 0;
        for (int seat = 0; seat < DRIVER_SEAT_LIMIT; seat++) {
            const SimDriver *driver = &race.drivers[seat];
            if (driver->status == SIM_EMPTY) continue;
            if (driver->status != SIM_DRIVER_FINISHED || driver->place < 1 ||
                driver->place > entrants || (places & (1u << driver->place))) return 0;
            places |= 1u << driver->place;
            for (int other = 0; other < DRIVER_SEAT_LIMIT; other++) {
                const SimDriver *competitor = &race.drivers[other];
                if (competitor->status == SIM_DRIVER_FINISHED && competitor->place < driver->place &&
                    competitor->finishTick > driver->finishTick) return 0;
            }
            u32 total = 0;
            for (int lap = 0; lap < PLAYER_LAP_TIME_CAPACITY; lap++) {
                if (lap < laps) {
                    if (driver->lapTicks[lap] == 0) return 0;
                    total += driver->lapTicks[lap];
                } else if (driver->lapTicks[lap] != 0) return 0;
            }
            if (driver->car.lap != laps + 1 || total > driver->finishTick) return 0;
        }
        restored = race;
        if (StepRaceSim(&race) || memcmp(&race, &restored, sizeof(race)) != 0) return 0;
        printf("complete race: laps=%d reverse=%d entrants=%d ticks=%u,%u places=%d,%d\n",
            laps, reverse, entrants, first->lapTicks[0], second->lapTicks[0], first->place, second->place);
        return 1;
    }
    if (race.elapsed != 500) return 0;
    for (int seat = 0; seat < humans; seat++) {
        if (!moved[seat] || race.drivers[seat].status != SIM_DRIVING) {
            fprintf(stderr, "short race driver stopped: model=%d reverse=%d seat=%d status=%d\n",
                    model, reverse, seat, race.drivers[seat].status); return 0;
        }
    }
    return 1;
}
static int StepRetailRival(const TrackData *track, s32 slot, int reverse) {
    GameCarRuntime car;
    const TrackEventData *events = track->events;
    if (!InitRival(&car, &track->route, &events->rivalStarts[reverse][slot + 1],
        events->trackWalkStart, reverse, (u16)slot)) return 0;
    ConfigureRival(&car, events->rivalAiConfigs[reverse], car.modelIndex, track->route.length, slot);
    SeedRivalSpeedKey(&car, events->aiSpeedKeys[reverse]);
    const GameCarRuntime initial = car;
    for (int tick = 0; tick < 250; tick++) {
        GameCarRuntime restored = car;
        if (!AdvanceRival(&car, &track->route, events, slot, reverse, NULL, 0) ||
            !AdvanceRival(&restored, &track->route, events, slot, reverse, NULL, 0)) return 0;
        FinishRival(&car, events, track->route.length, reverse);
        FinishRival(&restored, events, track->route.length, reverse);
        if (memcmp(&car, &restored, sizeof(car)) != 0 ||
            (u32)car.trackPointIndex >= (u32)track->route.count) return 0;
    }
    if (initial.activeFlag == -1) return memcmp(&car, &initial, sizeof(car)) == 0;
    return car.x != initial.x || car.z != initial.z;
}

int main(int argc, char **argv) {
    if (argc < 2 || argc > 4) { fprintf(stderr, "usage: retail_data_tests <CUE or Track 01 BIN> [track pack index, grid, or all [catalog.toml]]\n"); return 2; }
    const int gridOnly = argc >= 3 && strcmp(argv[2], "grid") == 0;
    if (gridOnly && argc != 3) return 2;
    long selected = -1;
    if (argc >= 3 && !gridOnly && strcmp(argv[2], "all") != 0) {
        char *end;
        selected = strtol(argv[2], &end, 10);
        if (*end != 0 || selected < 88 || selected > 134 || selected % 2) return 2;
    }
    RageCarCatalog catalog = {0};
    if (argc == 4) {
        char error[256];
        if (!LoadCarCatalog(argv[3], &catalog, error, sizeof(error))) {
            fprintf(stderr, "catalog: %s\n", error); return 2;
        }
    }
    RaceData *archive = LoadRaceDisc(argv[1]);
    if (archive == NULL) return 3;
    if (archive->boot[0] == '\0') {
        fprintf(stderr, "disc boot serial could not be read\n");
        FreeRaceData(archive);
        return 3;
    }
    printf("disc boot: %s\n", archive->boot);
    int tracks = 0, cars = 0;
    GameCarSpec specifications[32];
    for (int variant = 0; variant < CAR_MODEL_VARIANT_COUNT; variant++) {
        CarShape shape;
        if (!ReadRaceCar(archive, variant, &specifications[variant]) ||
            !ReadRaceCarShape(archive, variant, &shape)) {
            fprintf(stderr, "invalid car variant %d\n", variant); return 6;
        }
        CarModelData *model = CopyRaceCarModel(archive, variant);
        if (model == NULL || memcmp(&model->shape, &shape, sizeof(shape)) != 0) {
            fprintf(stderr, "invalid car model %d\n", variant);
            FreeCarModelData(model);
            return 6;
        }
        FreeCarModelData(model);
        cars++;
    }
    for (int index = 88; index < 135; index += 2) {
        if (selected >= 0 && selected != index) continue;
        const s32 pack = (index - 88) / 2;
        TrackData *track = CopyRaceTrack(archive, pack / 4, pack % 4);
        if (track && !CheckRetailGrid(archive, track)) {
            fprintf(stderr, "retail grid construction failed: asset=%d\n", index);
            FreeTrackData(track); FreeRaceData(archive); return 15;
        }
        if (!track) { fprintf(stderr, "invalid track asset %d\n", index); return 7; }
        printf("track %d: points=%d length=%d\n", index, track->route.count, track->route.length);
        if (gridOnly) { FreeTrackData(track); tracks++; continue; }
        for (int slot = 0; slot < RACE_CAR_SLOT_COUNT; slot++) {
            for (int reverse = 0; reverse < 2; reverse++) {
                if (!StepRetailRival(track, slot, reverse)) {
                    fprintf(stderr, "rival step failed: asset=%d slot=%d reverse=%d\n", index, slot, reverse); return 10;
                }
            }
        }
        for (int reverse = 0; reverse < 2; reverse++) {
            if (!StepRetailRace(track, &specifications[0], reverse, 0, 0, 1, 0, DRIVER_SEAT_LIMIT)) {
                fprintf(stderr, "full grid failed: asset=%d reverse=%d\n", index, reverse); return 15;
            }
            if (!StepRetailRace(track, &specifications[0], reverse, 0, 1, 1, 23, 2)) {
                fprintf(stderr, "mixed race step failed: asset=%d reverse=%d\n", index, reverse); return 11;
            }
        }
        for (int variant = 0; variant < cars; variant++) {
            for (int reverse = 0; reverse < 2; reverse++) {
                if (!StepRetailRace(track, &specifications[variant], reverse, 0, 0, 1, 23, 2)) {
                    fprintf(stderr, "race step failed: asset=%d variant=%d reverse=%d\n",
                        index, variant, reverse); return 8;
                }
            }
        }
        for (size_t entry = 0; entry < catalog.count; entry++) {
            const RageCarCatalogEntry *override = &catalog.entries[entry];
            const int variant = CarCatalogVariant(override->modelIndex, override->grade);
            GameCarSpec automatic = specifications[variant];
            ApplyCarSpec(override, &automatic);
            for (int reverse = 0; reverse < 2; reverse++) {
                if (!StepRetailRace(track, &automatic, reverse, 0, 0, 0,
                                    (s16)override->modelIndex, 2)) {
                    fprintf(stderr, "automatic catalog race failed: asset=%d variant=%d reverse=%d\n",
                            index, variant, reverse); return 13;
                }
            }
            GameCarSpec source;
            if (!ReadRaceCar(archive, variant, &source) ||
                memcmp(&source, &specifications[variant], sizeof(source)) != 0) return 14;
        }
        if (index == 88 || index == 90 || index == 92 || index == 102) {
            for (int reverse = 0; reverse < 2; reverse++) {
                for (int laps = 1; laps <= (index == 102 ? PLAYER_LAP_TIME_CAPACITY : 1); laps++) {
                    if (!StepRetailRace(track, &specifications[0], reverse, laps, 0, 1, 23, 2)) return 9;
                    if (!StepRetailRace(track, &specifications[0], reverse, laps, 1, 1, 23, 2)) {
                        fprintf(stderr, "mixed race incomplete: asset=%d reverse=%d laps=%d\n", index, reverse, laps); return 12;
                    }
                }
            }
        }
        FreeTrackData(track); tracks++;
    }
    if (gridOnly) {
        printf("retail grid: %d specifications, %d tracks, both directions and full/mixed fields passed\n", cars, tracks);
        FreeRaceData(archive); return 0;
    }
    printf("retail data: %d car specifications, %d tracks; two-human, mixed and %d-human field sweeps passed\n", cars, tracks, DRIVER_SEAT_LIMIT);
    FreeRaceData(archive); return 0;
}
