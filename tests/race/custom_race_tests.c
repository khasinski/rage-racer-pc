#include "game/car.h"
#include "game/race.h"

#include <stdio.h>

RaceSession g_RaceSession;
s32 g_CourseIndex;
s16 g_GrandPrixSeries;
s32 g_GrandPrixClass;
s32 g_PlayerCarIndex;
CarEntry *g_CarTable;
u8 g_CarModelBaseIndex[GAME_CAR_COUNT] = {0,4,7,9,14,18,21,23,26,28,29,30,31};
u8 g_CarModelUnlockBase[GAME_CAR_COUNT] = {1,2,3,0,1,2,3,2,3,4,5,5,5};

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)

int main(void) {
    CHECK(CustomRacePerformanceCar(0, 0, 0) == 3);
    CHECK(CustomRacePerformanceCar(3, 0, 1) == 6);
    CHECK(CustomRacePerformanceCar(3, 0, 2) == 2);
    CHECK(CustomRacePerformanceCar(0, 5, 0) == 11);
    CHECK(CustomRacePerformanceCar(0, 5, 1) == 11);
    CHECK(CustomRacePerformanceCar(0, 5, 2) == 10);
    for (s32 classIndex = 0; classIndex < 6; ++classIndex) {
        for (s32 model = 3; model < RACE_CAR_SLOT_COUNT; ++model) {
            CHECK(CustomRacePerformanceCar(0, classIndex, model) == 3);
        }
    }

    g_RaceSession = (RaceSession){RACE_SESSION_CUSTOM, 6, 4, 18};
    ApplyCustomRaceSelection();
    CHECK(g_CourseIndex == 2 && g_GrandPrixSeries == 1);
    CHECK(g_GrandPrixClass == 4 && g_PlayerCarIndex == 3);
    CHECK(CustomRaceUsesRivalModel() && CustomRaceRivalModel() == 5);

    g_RaceSession.model = 12;
    ApplyCustomRaceSelection();
    CHECK(g_PlayerCarIndex == 12 && !CustomRaceUsesRivalModel());
    puts("custom race tests passed");
    return 0;
}
