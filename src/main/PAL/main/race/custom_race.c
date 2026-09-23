#include "game/car.h"
#include "game/course_index.h"
#include "game/race.h"

#include <string.h>

/* The first three track-bank bodies correspond to named player cars.  The
 * table was recovered from the authored wheel matches for every class/course
 * pack.  The remaining rival-only bodies use GNADE (Esperanza) handling. */
static const u8 s_namedBodyCars[6][4][4] = {
    {{3,3,3,3}, {3,3,3,3}, {3,3,3,3}, {3,6,2,9}},
    {{3,4,0,3}, {3,4,0,3}, {3,4,0,3}, {3,5,1,7}},
    {{3,5,1,7}, {3,5,1,7}, {3,5,1,7}, {3,5,1,7}},
    {{3,6,2,8}, {3,6,2,8}, {3,6,2,8}, {3,6,2,8}},
    {{3,6,2,9}, {3,6,2,9}, {3,6,2,9}, {3,6,2,9}},
    {{11,11,10,12}, {11,11,10,12}, {11,11,10,12}, {11,11,10,12}},
};

int CustomRaceUsesRivalModel(void) {
    return g_RaceSession.kind == RACE_SESSION_CUSTOM &&
           g_RaceSession.model >= GAME_CAR_COUNT;
}

s32 CustomRaceRivalModelForSelection(s32 selection) {
    return selection >= GAME_CAR_COUNT ? selection - GAME_CAR_COUNT : -1;
}

s32 CustomRaceRivalModel(void) {
    return CustomRaceUsesRivalModel()
               ? CustomRaceRivalModelForSelection(g_RaceSession.model)
               : -1;
}

enum { CUSTOM_RACE_OVAL_MINIMUM_CLASS = 2 };

s32 CustomRacePerformanceCar(s32 course, s32 classIndex, s32 rivalModel) {
    if ((u32)course >= 4 || (u32)classIndex >= 6 ||
        (u32)rivalModel >= RACE_CAR_SLOT_COUNT) return 3;
    if (rivalModel < 3) {
        return s_namedBodyCars[classIndex][course][rivalModel];
    }
    return 3;
}

s32 CustomRacePreviewCar(s32 model) {
    if ((u32)model < GAME_CAR_COUNT) return model;
    return CustomRacePerformanceCar(
        g_RaceSession.course % COURSE_SLOT_COUNT,
        g_RaceSession.classIndex,
        model - GAME_CAR_COUNT);
}

s32 CustomRaceModelCount(s32 classIndex) {
    return classIndex == GRAND_PRIX_FINAL_CLASS_INDEX
               ? GAME_CAR_COUNT + 4
               : CUSTOM_RACE_MODEL_COUNT;
}

void ApplyCustomRaceSelection(void) {
    s32 course = g_RaceSession.course;
    s32 model = g_RaceSession.model;
    s32 modelCount = CustomRaceModelCount(g_RaceSession.classIndex);

    if (model < 0) model = 0;
    if (model >= modelCount) model = modelCount - 1;
    g_RaceSession.model = model;

    /* The Extreme Oval only exists from class 3 up. Below that the slot has
     * no track data and the race would load the first course with a broken
     * grid, so fall back to the first course of the same series. */
    if (g_RaceSession.classIndex < CUSTOM_RACE_OVAL_MINIMUM_CLASS &&
        CourseSlot(course) == COURSE_LONG_SLOT) {
        course -= COURSE_LONG_SLOT;
        g_RaceSession.course = course;
    }
    g_CourseIndex = course % COURSE_SLOT_COUNT;
    g_GrandPrixSeries = course / COURSE_SLOT_COUNT;
    g_GrandPrixClass = g_RaceSession.classIndex;
    g_PlayerCarIndex = CustomRacePreviewCar(model);
    for (s32 car = 0; car < GAME_CAR_COUNT; ++car) {
        s32 first = g_CarModelBaseIndex[car];
        s32 end = car + 1 < GAME_CAR_COUNT
                      ? g_CarModelBaseIndex[car + 1]
                      : CAR_MODEL_VARIANT_COUNT;
        s32 grade = g_GrandPrixClass - g_CarModelUnlockBase[car];
        if (grade < 0) grade = 0;
        if (grade >= end - first) grade = end - first - 1;
        g_RaceSession.cars[car].modelVariant = (u8)grade;
    }
    g_CarTable = g_RaceSession.cars;
}
