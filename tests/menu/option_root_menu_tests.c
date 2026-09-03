#include "common.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/render_state.h"

#include <stdio.h>
#include <string.h>

s32 g_ClassRecordMenuCursor;
s32 g_CourseIndex;
GameFrameContext *g_DrawBuffer;
s32 g_GameMode;
s32 g_GrandPrixClass;
s16 g_GrandPrixMode;
s16 g_GrandPrixSeries;
s32 g_OptionMenuCursor;
u16 g_PadPressed;
GameRenderState g_RenderState;
s32 g_ScreenOffsetEditX;
s32 g_ScreenOffsetEditY;
ScreenOffset g_ScreenOffsetX;
ScreenOffset g_ScreenOffsetY;
s32 g_SoundOptionCursor;

typedef struct LabelRecord {
    s32 x;
    s32 y;
    s32 width;
    s32 u;
    s32 v;
} LabelRecord;

static GameFrameContext s_frame;
static u8 s_packets[128];
static LabelRecord s_labels[6];
static s32 s_labelCount;
static s32 s_lastCue;
static s32 s_lastExitScene;
static s32 s_cursorCalls;
static s32 s_controllerConfigCalls;
static s32 s_trackLoadCalls;
static s32 s_randomValues[3];
static s32 s_randomIndex;
static s32 s_drawMode;

u8 *GameQueueSpriteTrans(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y, s32 width,
                         s32 height, s32 u, s32 v, s32 clut) {
    (void)ot;
    (void)height;
    (void)clut;
    s_labels[s_labelCount++] = (LabelRecord){x, y, width, u, v};
    return prim + 1;
}

u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *prim, s32 texturePage) {
    (void)ot;
    s_drawMode = texturePage;
    return prim + 1;
}

void DrawMenuCursorArrow(s32 x, s32 y) {
    (void)x;
    (void)y;
    s_cursorCalls++;
}

void PlaySoundCue(s32 cue) { s_lastCue = cue; }
void BeginControllerConfig(void) { s_controllerConfigCalls++; }
s32 Random15(void) { return s_randomValues[s_randomIndex++]; }
s32 RequestCourseTextureAssets(void) {
    s_trackLoadCalls++;
    return 0;
}
void StartOptionMenuExit(u32 scene) { s_lastExitScene = (s32)scene; }

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,         \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void Reset(void) {
    memset(&s_frame, 0, sizeof(s_frame));
    g_DrawBuffer = &s_frame;
    g_RenderState.packetCursor = s_packets;
    g_GameMode = OPTION_MODE_ROOT;
    g_OptionMenuCursor = 0;
    g_PadPressed = 0;
    g_ClassRecordMenuCursor = -1;
    g_ScreenOffsetEditX = -1;
    g_ScreenOffsetEditY = -1;
    g_ScreenOffsetX.value = 23;
    g_ScreenOffsetY.value = -17;
    g_SoundOptionCursor = -1;
    s_labelCount = 0;
    s_lastCue = 0;
    s_lastExitScene = -1;
    s_cursorCalls = 0;
    s_controllerConfigCalls = 0;
    s_trackLoadCalls = 0;
    s_randomIndex = 0;
    s_drawMode = -1;
}

int main(void) {
    static const LabelRecord expected[6] = {
        {0x24, 0x94, 0x3C, 0x00, 0x48},
        {0x24, 0xB4, 0x88, 0x40, 0x48},
        {0x24, 0xD4, 0x74, 0x00, 0x60},
        {0x24, 0xF4, 0x5C, 0x74, 0x60},
        {0x24, 0x114, 0x64, 0x00, 0x78},
        {0x24, 0x134, 0x1C, 0xD0, 0x60},
    };
    s32 cursor;

    Reset();
    DrawOptionRootMenu();
    CHECK(s_labelCount == 6);
    CHECK(memcmp(s_labels, expected, sizeof(expected)) == 0);
    CHECK(s_drawMode == 0x3F && s_cursorCalls == 1);

    Reset();
    g_OptionMenuCursor = 0;
    g_PadPressed = PAD_UP;
    UpdateOptionRootMenu();
    CHECK(g_OptionMenuCursor == 5 && s_lastCue == 1);

    for (cursor = 0; cursor < 6; cursor++) {
        Reset();
        g_OptionMenuCursor = cursor;
        g_PadPressed = PAD_CONFIRM;
        UpdateOptionRootMenu();
        CHECK(s_lastCue == 2);
        if (cursor == 0) {
            CHECK(g_GameMode == OPTION_MODE_CLASS_MENU &&
                  g_ClassRecordMenuCursor == 0);
            CHECK(g_ScreenOffsetEditX == 0 && g_ScreenOffsetEditY == 0);
        } else if (cursor == 1) {
            CHECK(g_GameMode == OPTION_MODE_CONTROLLER_CONFIG &&
                  s_controllerConfigCalls == 1);
        } else if (cursor == 2) {
            CHECK(g_GameMode == OPTION_MODE_SOUND_MENU &&
                  g_SoundOptionCursor == 0);
        } else if (cursor == 3) {
            CHECK(s_trackLoadCalls == 1 && s_lastExitScene == 0x1B);
        } else if (cursor == 4) {
            CHECK(g_GameMode == OPTION_MODE_SCREEN_ADJUST);
            CHECK(g_ScreenOffsetEditX == 23 && g_ScreenOffsetEditY == -17);
        } else {
            CHECK(s_lastExitScene == 2);
        }
    }

    Reset();
    g_OptionMenuCursor = 3;
    s_randomValues[0] = 0;
    s_randomValues[1] = 3;
    s_randomValues[2] = 2;
    g_PadPressed = PAD_CONFIRM;
    UpdateOptionRootMenu();
    CHECK(g_GrandPrixMode == 0 && g_GrandPrixSeries == 0);
    CHECK(g_GrandPrixClass == 0 && g_CourseIndex == 2);
    CHECK(s_randomIndex == 3);

    Reset();
    g_PadPressed = PAD_CANCEL;
    UpdateOptionRootMenu();
    CHECK(s_lastCue == 3 && s_lastExitScene == 2);

    puts("option root menu preserves rendering and all navigation paths");
    return 0;
}
