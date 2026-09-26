#include "game/race_grid.h"
#include "game/asset_index.h"
#include "driver_fixture.h"
#include <stdio.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void) {
    PlayerCarRuntime car;
    GameCarSpec spec;
    CarPerformance engine;
    PrepareDriver(&car, &spec, &engine);
    const size_t size = sizeof(RaceCarAssetHeader) + sizeof(spec) + 4;
    u8 bytes[sizeof(RaceCarAssetHeader) + sizeof(spec) + 4] = {0};
    RaceCarAssetHeader header = {sizeof(header), sizeof(header) + sizeof(spec),
        sizeof(header) + sizeof(spec) + 1, sizeof(header) + sizeof(spec) + 2,
        sizeof(header) + sizeof(spec) + 3};
    memcpy(bytes, &header, sizeof(header));
    memcpy(bytes + sizeof(header), &spec, sizeof(spec));
    RaceData archive = {.data = bytes, .size = size};
    archive.entries[ASSET_CAR_2ND_BASE].size = (u32)size;
    GameTrackPoint points[2] = {{.segmentLength = 1000, .leftHalfWidth = 300, .rightHalfWidth = 300},
        {.x = 1000, .segmentLength = 1000, .leftHalfWidth = 300, .rightHalfWidth = 300}};
    TrackEventData events = {0};
    TrackData track = {.route = {.points = points, .count = 2, .length = 2000}, .events = &events};
    RaceEntrant entrants[DRIVER_SEAT_LIMIT] = {0};
    entrants[2] = (RaceEntrant){.kind = RACE_SEAT_HUMAN, .grid = 0, .manual = 1, .seed = 17};
    entrants[11] = (RaceEntrant){.kind = RACE_SEAT_AI, .grid = 1, .model = 7, .rivalSlot = 3, .seed = 29};
    RaceSim race;
    memset(&race, 0xa5, sizeof(race));
    CHECK(InitRaceGrid(&race, &archive, &track, entrants, NULL, 2, 1));
    CHECK(race.phase == SIM_SETUP && race.reverse == 1 && race.laps == 2);
    CHECK(race.drivers[2].status == SIM_DRIVING && !race.drivers[2].rival);
    CHECK(race.drivers[2].variant == 0 && race.drivers[11].variant == -1);
    CHECK(race.drivers[2].car.drive.manual == 1 && race.drivers[2].random == 17);
    CHECK(race.drivers[11].rival && race.drivers[11].rivalSlot == 3 && race.drivers[11].random == 29);
    CHECK(race.drivers[11].car.modelIndex == 7 && race.drivers[0].status == SIM_EMPTY);
    CHECK(race.route.points == points && race.events == &events);
    const RaceSim installed = race;
    entrants[11].grid = 12;
    CHECK(!InitRaceGrid(&race, &archive, &track, entrants, NULL, 2, 1));
    CHECK(memcmp(&race, &installed, sizeof(race)) == 0);
    entrants[11].grid = 0;
    CHECK(!InitRaceGrid(&race, &archive, &track, entrants, NULL, 2, 1));
    entrants[11].grid = 1;
    events.rivalStarts[1][1].activeFlag = -1;
    CHECK(!InitRaceGrid(&race, &archive, &track, entrants, NULL, 2, 1));
    events.rivalStarts[1][1].activeFlag = 0;
    entrants[2].model = 31;
    CHECK(!InitRaceGrid(&race, &archive, &track, entrants, NULL, 2, 1));
    entrants[2].model = 0;
    CHECK(!InitRaceGrid(&race, &archive, &track, entrants, NULL, 2, 2));
    CHECK(!InitRaceGrid(NULL, &archive, &track, entrants, NULL, 2, 1));
    CHECK(!InitRaceGrid(&race, NULL, &track, entrants, NULL, 2, 1));
    CHECK(!InitRaceGrid(&race, &archive, NULL, entrants, NULL, 2, 1));
    CHECK(!InitRaceGrid(&race, &archive, &track, NULL, NULL, 2, 1));
    CHECK(memcmp(&race, &installed, sizeof(race)) == 0);
entrants[10] = (RaceEntrant){.kind = RACE_SEAT_AI, .grid = 2, .model = 1, .rivalSlot = 3};
CHECK(!InitRaceGrid(&race, &archive, &track, entrants, NULL, 2, 1));
entrants[10] = (RaceEntrant){0};
entrants[2].manual = 2;
CHECK(!InitRaceGrid(&race, &archive, &track, entrants, NULL, 2, 1));
entrants[2].manual = 1;
entrants[11].kind = (RaceSeatKind)3;
CHECK(!InitRaceGrid(&race, &archive, &track, entrants, NULL, 2, 1));
entrants[11].kind = RACE_SEAT_AI;
RaceEntrant empty[DRIVER_SEAT_LIMIT] = {0};
CHECK(!InitRaceGrid(&race, &archive, &track, empty, NULL, 2, 1));
CHECK(memcmp(&race, &installed, sizeof(race)) == 0);
    RageCarCatalog catalog = {.count = 1};
    catalog.entries[0] = (RageCarCatalogEntry){.id = "test", .modelIndex = 0, .grade = 0,
        .fields = RAGE_CAR_FIELD_ID | RAGE_CAR_FIELD_MODEL | RAGE_CAR_FIELD_GRADE | RAGE_CAR_FIELD_REV_LIMIT,
        .specification = {.revLimit = 12000}};
    CHECK(InitRaceGrid(&race, &archive, &track, entrants, &catalog, 2, 1));
    CHECK(race.drivers[2].spec.revLimit == 12000);
    CHECK(race.drivers[2].spec.redline == spec.redline);
    GameCarSpec source;
    CHECK(ReadRaceCar(&archive, 0, &source) && memcmp(&source, &spec, sizeof(spec)) == 0);
    memset(bytes, 0, sizeof(bytes)); memset(&catalog, 0, sizeof(catalog));
    CHECK(race.drivers[2].spec.revLimit == 12000 && race.drivers[2].spec.redline == spec.redline);
    return 0;
}
