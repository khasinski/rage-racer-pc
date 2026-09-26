#include "game/race_lap.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); return 1; \
} } while (0)

int main(void) {
    PlayerCarRuntime first = {0}, second = {0};
    first.progressA = -1;
    CHECK(AdvanceCarLap(&first, 1000, 2) == LAP_NONE && first.lap == 0);
    first.progressA = 0;
    CHECK(AdvanceCarLap(&first, 1000, 2) == LAP_STARTED && first.lap == 1);
    first.progressA = 999;
    CHECK(AdvanceCarLap(&first, 1000, 2) == LAP_NONE);
    first.progressB = 1;
    CHECK(AdvanceCarLap(&first, 1000, 2) == LAP_COMPLETED && first.lap == 2);
    first.progressA = 2000;
    first.progressB = 0;
    CHECK(AdvanceCarLap(&first, 1000, 2) == LAP_FINISHED && first.lap == 3);
    PlayerCarRuntime finished = first;
    CHECK(AdvanceCarLap(&first, 1000, 2) == LAP_NONE);
    CHECK(memcmp(&first, &finished, sizeof(first)) == 0);
    CHECK(AdvanceCarLap(&second, 500, 1) == LAP_STARTED && second.lap == 1);
    second.progressB = 500;
    CHECK(AdvanceCarLap(&second, 500, 1) == LAP_FINISHED && second.lap == 2);
    CHECK(first.lap == 3);
    for (int laps = 1; laps <= PLAYER_LAP_TIME_CAPACITY; laps++) {
        for (int lap = 0; lap <= laps; lap++) {
            first = (PlayerCarRuntime){.lap = lap};
            first.progressA = lap * 1000 - 1;
            CHECK(AdvanceCarLap(&first, 1000, laps) == LAP_NONE);
            first.progressB = 1;
            LapEvent expected = lap == 0 ? LAP_STARTED : lap == laps ? LAP_FINISHED : LAP_COMPLETED;
            CHECK(AdvanceCarLap(&first, 1000, laps) == expected);
            CHECK(first.lap == lap + 1);
        }
    }
    first = (PlayerCarRuntime){.lap = 2, .progressA = INT_MAX, .progressB = INT_MAX};
    CHECK(AdvanceCarLap(&first, INT_MAX, 3) == LAP_COMPLETED);
    first = (PlayerCarRuntime){.progressA = -999};
    CHECK(!IsCarLapBehind(&first, 1000));
    first.progressB = -1;
    CHECK(IsCarLapBehind(&first, 1000));
    first.progressA = INT_MIN; first.progressB = INT_MIN;
    CHECK(IsCarLapBehind(&first, INT_MAX));
    CHECK(!IsCarLapBehind(NULL, 1000) && !IsCarLapBehind(&first, 0));
    first = (PlayerCarRuntime){0};
    CHECK(AdvanceCarLap(&first, 0, 3) == LAP_NONE);
    CHECK(AdvanceCarLap(&first, 1000, 0) == LAP_NONE);
    CHECK(AdvanceCarLap(&first, 1000, PLAYER_LAP_TIME_CAPACITY + 1) == LAP_NONE);
    CHECK(AdvanceCarLap(NULL, 1000, 3) == LAP_NONE);
    CHECK(first.lap == 0);
    return 0;
}
