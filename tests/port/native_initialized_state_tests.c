#include <stdio.h>
#include <string.h>

#include "game/asset.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/cd_internal.h"
#include "game/input_internal.h"
#include "game/memcard.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/race_hud_internal.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"

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

static void CheckInitialProportionalFont(void) {
    const ProportionalFontCell *capitalA =
        &g_PropFontCells['A' - 0x20];

    Check(HashBytes(g_PropFontCells, sizeof(g_PropFontCells)) == 3341930809u,
          "proportional font cell bytes");
    Check(capitalA->textureU == 0x78 && capitalA->textureV == 0x78,
          "proportional capital-A cell");
    Check(g_PropFontCells[PROPORTIONAL_FONT_CELL_COUNT - 1].textureU == 0x34 &&
              g_PropFontCells[PROPORTIONAL_FONT_CELL_COUNT - 1].textureV ==
                  0x90,
          "proportional final font cell");
}

static void CheckInitialAtanTable(void) {
    int index;

    Check(HashBytes(g_AtanTable, sizeof(g_AtanTable)) == 1890884459u,
          "arctangent table bytes");
    Check(g_AtanTable[0] == 0 &&
              g_AtanTable[ATAN_TABLE_SAMPLE_COUNT - 1] == 0x200,
          "arctangent table endpoints");
    Check(g_AtanTable[ATAN_TABLE_SAMPLE_COUNT] == 0,
          "arctangent table retail padding");
    for (index = 1; index < ATAN_TABLE_SAMPLE_COUNT; index++) {
        Check(g_AtanTable[index] >= g_AtanTable[index - 1],
              "arctangent table is monotonic");
    }
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

static void CheckInitialControllerMappings(void) {
    Check(HashBytes(g_PadButtonPresets, sizeof(g_PadButtonPresets)) ==
              2570916421u,
          "pad button preset bytes");
    Check(HashBytes(g_NegconButtonPresets,
                    sizeof(g_NegconButtonPresets)) == 2254807281u,
          "NeGcon button preset bytes");
    Check(g_PadButtonPresets[0][0] == PAD_LEFT &&
              g_PadButtonPresets[0][1] == PAD_RIGHT &&
              g_PadButtonPresets[7][7] == PAD_SQUARE,
          "pad button preset coordinates");
    Check(g_NegconButtonPresets[0][4] == PAD_DOWN &&
              g_NegconButtonPresets[7][6] == PAD_SQUARE &&
              g_NegconButtonPresets[7][7] == PAD_CROSS,
          "NeGcon button preset coordinates");
}

static void CheckInitialCountdownData(void) {
    static const StartCountdownPattern expectedPatterns = {
        0x00000000, 0x00000000, 0x3FFF3FFF, 0x3FFF3FFF,
        0x3FFF3FFF, 0x3C1F3C1F, 0x7C007C1E, 0x7CFE7C1E,
        0x78FE783E, 0x783E783E, 0xF83CF83C, 0xFFFCFFFC,
        0xFFFCFFFC, 0xFFFCFFFC, 0x00000000, 0x00000000,
    };
    static const StartCountdownColorBank
        expectedColors[START_COUNTDOWN_COLOR_BANK_COUNT] = {
            {{0xFF, 0x20, 0x00, 0x60}, {0x40, 0x10, 0x00, 0x60}},
            {{0x00, 0x40, 0xFF, 0x60}, {0x00, 0x10, 0x40, 0x60}},
    };
    int bank;
    int index;

    for (index = 0; index < START_COUNTDOWN_PATTERN_ROW_COUNT; index++) {
        Check(g_CountdownDigitPatterns[index] == expectedPatterns[index],
              "countdown digit pattern");
    }
    for (bank = 0; bank < START_COUNTDOWN_COLOR_BANK_COUNT; bank++) {
        for (index = 0; index < START_COUNTDOWN_COLORS_PER_BANK; index++) {
            Check(memcmp(&g_CountdownCellColors[bank][index],
                         &expectedColors[bank][index], sizeof(CVec)) == 0,
                  "countdown cell colour");
        }
    }
}

static void CheckMatrix(const Matrix *actual, const s16 expected[3][3],
                        const char *message) {
    int row;
    int column;

    for (row = 0; row < 3; row++) {
        for (column = 0; column < 3; column++) {
            Check(actual->m[row][column] == expected[row][column], message);
        }
        Check(actual->t[row] == 0, message);
    }
}

static void CheckInitialLightingMatrices(void) {
    static const s16 trackColor[3][3] = {
        {819, 0, 192}, {819, 0, 192}, {819, 0, 192},
    };
    static const s16 trackLight[3][3] = {
        {3072, -6144, 6144}, {7094, -7094, -7094}, {-7094, -7094, -7094},
    };
    static const s16 defaultColor[3][3] = {
        {4096, 4096, 4096}, {4096, 4096, 4096}, {4096, 4096, 4096},
    };
    static const s16 defaultLight[3][3] = {
        {0, 0, -2048}, {682, -1024, -512}, {-682, -1024, -512},
    };
    static const s16 menuColor[3][3] = {
        {3072, 4096, 4096}, {3072, 4096, 4096}, {3072, 4096, 4096},
    };
    static const s16 menuLight[3][3] = {
        {0, -1024, 2048}, {682, -1024, -512}, {-682, -1024, -512},
    };

    CheckMatrix(&g_TrackColorMatrix, trackColor, "track colour matrix");
    CheckMatrix(&g_TrackLightMatrix, trackLight, "track light matrix");
    CheckMatrix(&g_DefaultColorMatrix, defaultColor, "default colour matrix");
    CheckMatrix(&g_DefaultLightMatrix, defaultLight, "default light matrix");
    CheckMatrix(&g_MenuColorMatrix, menuColor, "menu colour matrix");
    CheckMatrix(&g_MenuLightMatrix, menuLight, "menu light matrix");
}

static void CheckInitialRaceHudSprites(void) {
    Check(HashBytes(g_RaceHudSpriteDescsGp,
                    sizeof(g_RaceHudSpriteDescsGp)) == 379897236u,
          "Grand Prix HUD sprite descriptor bytes");
    Check(HashBytes(g_RaceHudSpriteDescsTimeTrial,
                    sizeof(g_RaceHudSpriteDescsTimeTrial)) == 46032874u,
          "time-attack HUD sprite descriptor bytes");
    Check(g_RaceHudSpriteDescsGp[0].x == 240 &&
              g_RaceHudSpriteDescsGp[GRAND_PRIX_HUD_SPRITE_COUNT - 1].u0 ==
                  0xe8,
          "Grand Prix HUD sprite coordinates");
    Check(g_RaceHudSpriteDescsTimeTrial[TIME_ATTACK_HUD_SPRITE_COUNT - 1].x ==
                  120 &&
              g_RaceHudSpriteDescsTimeTrial
                  [TIME_ATTACK_HUD_SPRITE_COUNT - 1].clut == 0x78cc,
          "time-attack split-sign descriptor");
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
    static const char *const expectedPaths[MEMORY_CARD_SAVE_SLOT_COUNT] = {
        "bu00:BESCES-00650 RAGE000",
        "bu00:BESCES-00650 RAGE001",
        "bu00:BESCES-00650 RAGE002",
    };
    s32 slot;

    for (slot = 0; slot < MEMORY_CARD_SAVE_SLOT_COUNT; slot++) {
        const char *path = &g_SaveFilePath[slot * MC_SAVE_PATH_SIZE];
        const u8 *title =
            (const u8 *)&g_SaveTitleSjis[slot * MC_SAVE_TITLE_SIZE];

        Check(strcmp(path, expectedPaths[slot]) == 0,
              "memory-card save path row");
        Check(title[0] == 0x82 && title[1] == 0x71 &&
                  title[55] == (u8)(0x50 + slot) && title[56] == 0,
              "memory-card Shift-JIS title row");
    }
    Check(g_SaveFilePath[MEMORY_CARD_SAVE_SLOT_COUNT *
                             MC_SAVE_PATH_SIZE] == 0 &&
              g_SaveFilePath[MEMORY_CARD_SAVE_PATH_STORAGE_SIZE - 1] == 0,
          "memory-card save path table padding");
    Check(g_SaveTitleSjis[MEMORY_CARD_SAVE_SLOT_COUNT *
                              MC_SAVE_TITLE_SIZE] == 0 &&
              g_SaveTitleSjis[MEMORY_CARD_SAVE_TITLE_STORAGE_SIZE - 1] == 0,
          "memory-card save title table padding");
    Check(memcmp(g_SaveNameCharset,
                 "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ.-!?@",
                 SAVE_NAME_CHARACTER_COUNT) == 0 &&
              g_SaveNameCharset[SAVE_NAME_CHARACTER_COUNT] == '\0' &&
              g_SaveNameCharset[SAVE_NAME_CHARSET_STORAGE_SIZE - 1] == '\0',
          "save-name character set and padding");
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
    CheckInitialProportionalFont();
    CheckInitialAtanTable();
    CheckInitialCourseCarModels();
    CheckInitialControllerMappings();
    CheckInitialCountdownData();
    CheckInitialLightingMatrices();
    CheckInitialRaceHudSprites();
    CheckInitialStartingGrids();
    CheckMemoryCardLabels();
    CheckInitialAudioTables();

    if (s_failures != 0) return 1;
    puts("native initialized state retains its typed retail values");
    return 0;
}
