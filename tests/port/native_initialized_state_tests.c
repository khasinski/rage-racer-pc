#include <stdio.h>
#include <string.h>

#include "game/asset.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/cd_internal.h"
#include "game/memcard.h"
#include "game/race.h"
#include "game/render.h"

static int s_failures;

static void Check(int condition, const char *message) {
    if (condition) return;
    s_failures++;
    printf("FAIL %s\n", message);
}

static u32 HashBytes(const void *data, size_t size) {
    const u8 *bytes = data;
    u32 hash = 2166136261u;
    size_t index;

    for (index = 0; index < size; index++) {
        hash ^= bytes[index];
        hash *= 16777619u;
    }
    return hash;
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

static void CheckInitialPaintData(void) {
    Check(g_BodyColorPrimary[0] == 0xbe73 &&
              g_BodyColorPrimary[17] == 0xa5ab,
          "primary body colours");
    Check(g_BodyColorSecondary[0] == 0x9929 &&
              g_BodyColorSecondary[17] == 0x8ca4,
          "secondary body colours");
    Check(g_PaintSlots3StopA[0] == 0x0001 &&
              g_PaintSlots3StopA[8] == 0x0341 &&
              g_PaintSlots3StopA[9] == 0,
          "primary three-stop paint slots and padding");
    Check(g_PaintSlots3StopB[7] == 0x0341 &&
              g_PaintSlots4Stop[0] == 0x0141 &&
              g_PaintSlots4Stop[3] == 0x0401,
          "secondary paint slots");
}

static void CheckInitialCarModelBanks(void) {
    static const s16 expected[CAR_MODEL_BANK_ENTRY_COUNT]
                             [CAR_MODEL_BANK_FIELDS] = {
        {0, 0},   {5, 0},   {10, 0}, {15, 0},
        {20, 0},  {20, 1},  {25, 0}, {25, 1},
        {30, 0},  {30, 1},  {30, 2},
    };

    Check(memcmp(g_CarModelBankTable, expected, sizeof(expected)) == 0,
          "car model-bank entries");
}

static void CheckInitialCourseCarModels(void) {
    static const u8 expected[CAR_MODEL_COURSE_COUNT][RACE_CAR_SLOT_COUNT] = {
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
        {1, 2, 0, 3, 4, 5, 6, 7, 8, 9, 10},
        {2, 0, 1, 3, 4, 5, 6, 7, 8, 9, 10},
        {3, 0, 1, 2, 4, 5, 6, 7, 8, 9, 10},
    };

    Check(memcmp(g_CarModelByCourse, expected, sizeof(expected)) == 0,
          "course car-model ordering");
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

static void CheckMemoryCardLabels(void) {
    Check(g_McMessageColumnX[2] == 0x60 &&
              g_McMessageColumnX[3] == 0x78 &&
              g_McMessageColumnX[4] == 0xB4,
          "memory-card message columns");
    Check(strcmp(g_McSlotLabels, "NEW FILE") == 0,
          "new-file label");
    Check(strcmp(g_McSlotLabelNoFile, "NO FILE") == 0,
          "no-file label");
    Check(strcmp(g_McSlotLabelError, "FILE ERROR") == 0,
          "file-error label");
}

static void CheckInitialAudioTables(void) {
    Check(HashBytes(g_SoundCueParams, sizeof(g_SoundCueParams)) ==
              522356387u,
          "main sound cue table bytes");
    Check(HashBytes(g_SoundCueParams2, sizeof(g_SoundCueParams2)) ==
              2735735337u,
          "race sound cue table bytes");
    Check(HashBytes(g_EffectCueTable, sizeof(g_EffectCueTable)) ==
              2869815252u,
          "effect cue table bytes");
    Check(HashBytes(g_SpecialVoiceBits, sizeof(g_SpecialVoiceBits)) ==
              3282707433u,
          "special voice mask bytes");
    Check(HashBytes(g_VabSpuAddress, sizeof(g_VabSpuAddress)) == 41473987u,
          "VAB SPU address bytes");
    Check(HashBytes(g_CdTrackLoopPoint, sizeof(g_CdTrackLoopPoint)) ==
              380835909u,
          "CD track loop-point bytes");
    Check(g_EffectCueTable[0].voiceCount == 2 &&
              g_EffectCueTable[0].volumeScale == 85 &&
              g_EffectCueTable[2].programs[1].note == 26 &&
              g_EffectCueTable[2].programs[1].tone == 1,
          "effect cue banks");
    Check(g_SpecialVoiceBits[0] == 0x00040000 &&
              g_SpecialVoiceBits[5] == 0x00800000,
          "special voice masks");
    Check(g_VabSpuAddress[AUDIO_SLOT_MAIN_CUES] == 0x1000 &&
              g_VabSpuAddress[AUDIO_SLOT_ENGINE] == 0x6A000,
          "VAB SPU addresses");
    Check(g_CdTrackLoopPoint[2].minute == 0 &&
              g_CdTrackLoopPoint[3].minute == 5 &&
              g_CdTrackLoopPoint[11].minute == 5 &&
              g_CdTrackLoopPoint[12].minute == 0 &&
              g_CdTrackLoopPoint[17].minute == 5,
          "CD track loop points");
}

int main(void) {
    CheckInitialRectangles();
    CheckInitialPaintData();
    CheckInitialCarModelBanks();
    CheckInitialCourseCarModels();
    CheckInitialCountdownData();
    CheckInitialStartingGrids();
    CheckMemoryCardLabels();
    CheckInitialAudioTables();

    if (s_failures != 0) return 1;
    puts("native initialized state retains its typed retail values");
    return 0;
}
