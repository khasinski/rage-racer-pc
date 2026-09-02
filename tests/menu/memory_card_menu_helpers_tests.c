#include <stdio.h>
#include <string.h>

#include "game/asset.h"
#include "game/memcard.h"
#include "game/menu.h"
#include "game/state.h"

s32 g_AssetLoadState;
static u8 s_imageData[16];
u8 *g_ImageBlockBuffer = s_imageData;
s32 g_McFadeLevel;
s32 g_McFadeStep;
s32 g_McFreeBlocks;
s32 g_McFromLoadMenu;
s32 g_McMenuPage;
s32 g_McMenuRowCount;
s32 g_McMenuRowCursor;
s32 g_McMenuState;
s32 g_SceneId;
s32 g_SceneTimer;
u16 g_PadPressed;
u16 g_PadPressedRepeat;
char g_FmtSaveRow[] = "%d/";
char g_FmtSaveRowEmpty[] = "%d/EMPTY";
char g_FmtSaveRowTail[] = "/";
u8 g_SaveNameCharset[SAVE_NAME_CHARSET_SIZE];
char g_McSlotLabelError[] = "ERROR";
char g_McSlotLabelNoFile[] = "NO FILE";
char g_McSlotLabels[] = "NEW FILE";

typedef struct TextDraw {
    s32 x;
    s32 y;
    char text[24];
} TextDraw;

static TextDraw s_draws[12];
static s32 s_drawCount;
static s32 s_cues[4];
static s32 s_cueCount;
static s32 s_startEvents;
static s32 s_stopEvents;
static s32 s_displayMask;
static s32 s_displaySetup;
static s32 s_fadeColor;
static s32 s_fadeTpage;
static s32 s_imageUploads;
static s32 s_failures;

#define CHECK(condition)                                                                  \
    do {                                                                                  \
        if (!(condition)) {                                                               \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition); \
            s_failures++;                                                                 \
        }                                                                                 \
    } while (0)

void DrawLargeText(s32 x, s16 y, const char *text, u8 red, u8 green, u8 blue,
                   u16 clut, s32 flags) {
    (void)red;
    (void)green;
    (void)blue;
    (void)clut;
    (void)flags;
    s_draws[s_drawCount].x = x;
    s_draws[s_drawCount].y = y;
    snprintf(s_draws[s_drawCount].text,
             sizeof(s_draws[s_drawCount].text),
             "%s",
             text);
    s_drawCount++;
}

char *FormatSaveElapsedTime(char *text, u32 ticks) {
    (void)ticks;
    strcpy(text, "TIME");
    return text;
}

void PlaySoundCue(s32 cue) {
    s_cues[s_cueCount++] = cue;
}

void StartMemoryCardEvents(void) {
    s_startEvents++;
}

void StopMemoryCardEvents(void) {
    s_stopEvents++;
}

void SetDispMask(s32 mask) {
    s_displayMask = mask;
}

void SetupDisplay480(s32 r, s32 g, s32 b) {
    CHECK(r == 0 && g == 0 && b == 0);
    s_displaySetup++;
}

void DrawFullscreenFadeTile480(s32 color, s32 tpage) {
    s_fadeColor = color;
    s_fadeTpage = tpage;
}

void UploadImageAsset(GameImageAssetHeaderWord *asset) {
    CHECK(asset == GetImageAssetHeaderWords(g_ImageBlockBuffer));
    s_imageUploads++;
}

static void Reset(void) {
    s32 i;

    memset(s_draws, 0, sizeof(s_draws));
    for (i = 0; i < SAVE_NAME_CHARSET_SIZE; i++) {
        g_SaveNameCharset[i] = (u8)('A' + i % 26);
    }
    s_drawCount = 0;
    s_cueCount = 0;
    s_startEvents = 0;
    s_stopEvents = 0;
    s_displayMask = -1;
    s_displaySetup = 0;
    s_fadeColor = -1;
    s_fadeTpage = -1;
    s_imageUploads = 0;
    g_AssetLoadState = 0;
    g_McFreeBlocks = 1;
    g_McMenuPage = 1;
    g_McMenuRowCursor = 0;
    g_PadPressed = 0;
    g_PadPressedRepeat = 0;
}

static void TestSaveRows(void) {
    GameSaveHeaderRow rows[MEMORY_CARD_SAVE_SLOT_COUNT];

    Reset();
    memset(rows, 0, sizeof(rows));
    rows[0].fields.nameLength = 7;
    rows[0].fields.name[0] = 0;
    rows[0].fields.name[1] = 1;
    rows[0].fields.name[2] = 2;
    rows[0].fields.name[3] = 3;
    rows[0].fields.name[4] = 4;
    rows[0].fields.name[5] = 5;
    DrawMemoryCardSaveRows(1 | (0x10000 << 1), rows);
    CHECK(s_drawCount == 7);
    CHECK(strcmp(s_draws[0].text, "1/") == 0);
    CHECK(strcmp(s_draws[1].text, "ABCDEF/") == 0);
    CHECK(strcmp(s_draws[2].text, "TIME") == 0);
    CHECK(s_draws[3].x == 0x48 && strcmp(s_draws[3].text, "2/") == 0);
    CHECK(s_draws[4].x == 0x88 && strcmp(s_draws[4].text, "ERROR") == 0);
    CHECK(strcmp(s_draws[5].text, "3/") == 0);
    CHECK(strcmp(s_draws[6].text, "NEW FILE") == 0);

    Reset();
    memset(rows, 0, sizeof(rows));
    g_McFreeBlocks = 0;
    g_McMenuRowCursor = 1;
    DrawMemoryCardSaveRows(0, rows);
    CHECK(s_drawCount == 6);
    CHECK(strcmp(s_draws[1].text, "NO FILE") == 0);

    Reset();
    memset(rows, 0, sizeof(rows));
    rows[0].fields.nameLength = 1;
    rows[0].fields.name[0] = 0xFF;
    DrawMemoryCardSaveRows(1, rows);
    CHECK(strcmp(s_draws[1].text, "?     /") == 0);
}

static void TestMenuControls(void) {
    s32 value;

    Reset();
    value = 1;
    g_PadPressedRepeat = PAD_DOWN;
    AdjustMenuSelectionVertical(&value, 0, 2);
    CHECK(value == 2 && s_cueCount == 1 && s_cues[0] == 1);
    AdjustMenuSelectionVertical(&value, 0, 2);
    CHECK(value == 2 && s_cueCount == 1);

    g_PadPressedRepeat = PAD_LEFT;
    value = 0;
    SetMenuBinaryChoiceHorizontal(&value);
    CHECK(value == 1 && s_cueCount == 2);
    SetMenuBinaryChoiceHorizontal(&value);
    CHECK(value == 1 && s_cueCount == 2);

    g_PadPressed = PAD_START | PAD_TRIANGLE;
    CHECK(PollMenuConfirmInput() == PAD_START);
    CHECK(PollMenuBackInput() == PAD_TRIANGLE);
    CHECK(s_cues[s_cueCount - 2] == 2 && s_cues[s_cueCount - 1] == 3);
}

static void TestMenuLifecycle(void) {
    Reset();
    EnterMemoryCardMenu();
    CHECK(s_displayMask == 0 && s_displaySetup == 1 && s_startEvents == 1);
    CHECK(g_McMenuRowCount == 2 && g_McMenuState == -1);
    CHECK(g_McMenuPage == 0 && g_McMenuRowCursor == 0);
    CHECK(g_McFadeStep == -8 && g_McFadeLevel == 0xFF);
    CHECK(g_SceneId == 0x1A && g_SceneTimer == 0);

    StartMenuExitFade();
    CHECK(s_stopEvents == 1 && g_McFadeStep == 8);
    DrawMenuFadeOverlay(123);
    CHECK(s_fadeColor == 123 && s_fadeTpage == 0x40);

    Reset();
    g_AssetLoadState = 1;
    g_McMenuState = 99;
    EnterMemoryCardMenuFromLoad();
    CHECK(s_displayMask == 0 && s_displaySetup == 1);
    CHECK(s_imageUploads == 0 && s_startEvents == 0);
    CHECK(g_McMenuState == 99);

    g_AssetLoadState = 0;
    EnterMemoryCardMenuFromLoad();
    CHECK(s_displaySetup == 2 && s_imageUploads == 1 && s_startEvents == 1);
    CHECK(g_McMenuRowCount == 3 && g_McMenuRowCursor == 2);
    CHECK(g_McMenuState == -1 && g_McMenuPage == 0);
    CHECK(g_McFromLoadMenu == 1 && g_SceneTimer == 0);
    CHECK(g_McFadeStep == -8 && g_McFadeLevel == 0xFF);
    CHECK(g_SceneId == 0x1A);
}

int main(void) {
    TestSaveRows();
    TestMenuControls();
    TestMenuLifecycle();
    return s_failures != 0;
}
