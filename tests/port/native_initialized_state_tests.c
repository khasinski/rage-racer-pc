#include <stdio.h>

#include "game/asset.h"
#include "game/car.h"
#include "game/race.h"
#include "game/render.h"

static int s_failures;

static void Check(int condition, const char *message) {
    if (condition) return;
    s_failures++;
    printf("FAIL %s\n", message);
}

static void CheckInitialRectangles(void) {
    Check(g_CarImageRect.x == 704 && g_CarImageRect.y == 0 &&
              g_CarImageRect.w == 64 && g_CarImageRect.h == 256,
          "car image rectangle");
    Check(g_TrackTextureRowRect.x == 576 &&
              g_TrackTextureRowRect.y == 0 &&
              g_TrackTextureRowRect.w == 448 &&
              g_TrackTextureRowRect.h == 1,
          "track texture row rectangle");
}

static void CheckInitialCountdownData(void) {
    static const u32 expectedPatterns[16] = {
        0x00000000, 0x00000000, 0x3FFF3FFF, 0x3FFF3FFF,
        0x3FFF3FFF, 0x3C1F3C1F, 0x7C007C1E, 0x7CFE7C1E,
        0x78FE783E, 0x783E783E, 0xF83CF83C, 0xFFFCFFFC,
        0xFFFCFFFC, 0xFFFCFFFC, 0x00000000, 0x00000000,
    };
    static const CVec expectedColors[4] = {
        {0xFF, 0x20, 0x00, 0x60},
        {0x40, 0x10, 0x00, 0x60},
        {0x00, 0x40, 0xFF, 0x60},
        {0x00, 0x10, 0x40, 0x60},
    };
    int index;

    for (index = 0; index < 16; index++) {
        Check(g_CountdownDigitPatterns[index] == expectedPatterns[index],
              "countdown digit pattern");
    }
    for (index = 0; index < 4; index++) {
        Check(g_CountdownCellColors[index].r == expectedColors[index].r &&
                  g_CountdownCellColors[index].g == expectedColors[index].g &&
                  g_CountdownCellColors[index].b == expectedColors[index].b &&
                  g_CountdownCellColors[index].cd == expectedColors[index].cd,
              "countdown cell colour");
    }
}

static void CheckInitialStartingGrids(void) {
    int index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        Check(g_RaceGridSlots[index].value == index,
              "race starting-grid slot");
        Check(g_AttractGridSlots[index].value == index,
              "attract starting-grid slot");
    }
    Check(g_RaceGridSlots[RACE_CAR_SLOT_COUNT].value == -1,
          "race starting-grid terminator");
    Check(g_AttractGridSlots[RACE_CAR_SLOT_COUNT].value == -1,
          "attract starting-grid terminator");
}

int main(void) {
    CheckInitialRectangles();
    CheckInitialCountdownData();
    CheckInitialStartingGrids();

    if (s_failures != 0) return 1;
    puts("native initialized state retains its typed retail values");
    return 0;
}
