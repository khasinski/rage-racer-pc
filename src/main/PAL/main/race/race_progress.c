#include "game/car.h"
#include "game/course_index.h"
#include "game/race.h"

s32 StoreRaceSelection(GameRaceProgress *progress, s32 grandPrixMode,
                       s32 course, s32 carIndex, s32 classIndex, s32 money,
                       s32 timeAttackSeries) {
    if (progress == NULL || (u32)course >= COURSE_SLOT_COUNT ||
        (u32)carIndex >= GAME_CAR_COUNT ||
        (u32)classIndex > GRAND_PRIX_FINAL_CLASS_INDEX) {
        return 0;
    }

    progress->course = course;
    progress->carIndex = carIndex;
    progress->classIndex = classIndex;
    if (grandPrixMode != 0) {
        progress->money = money;
    } else {
        progress->timeAttackSeries = timeAttackSeries;
    }
    return 1;
}
