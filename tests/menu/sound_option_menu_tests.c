#include "common.h"
#include "game/audio.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/render_internal.h"
#include "game/render_state.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

s32 g_BgmVolumeSetting;
s32 g_GameMode;
s32 g_MonoOutput;
u16 g_PadPressed;
s32 g_SfxVolumeSetting;
GameFrameContext *g_DrawBuffer;
GameRenderState g_RenderState;

typedef struct ChoiceRecord {
    s32 v;
    s32 intensity;
} ChoiceRecord;

static GameFrameContext s_frame;
static SoundOption s_screen;
static u8 s_packets[128];
static ChoiceRecord s_choices[2];
static s32 s_choiceCount;
static s32 s_applyCalls;
static s32 s_lastCue;
static s32 s_lastHighlightX;
static s32 s_tileCount;
static s32 s_volumeCalls;
static s32 s_volumeLevels[2];

void DrawMenuCursorArrow(s32 x, s32 y) {
    (void)x;
    (void)y;
}
void DrawOptionHintBar(s32 variant) { (void)variant; }
void ApplyAudioSettings(void) { s_applyCalls++; }
void PlaySoundCue(s32 cue) { s_lastCue = cue; }
void DrawVolumeBar(s32 level, s32 y) {
    (void)y;
    s_volumeLevels[s_volumeCalls++] = level;
}

u8 *GameQueueSpriteTrans(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y, s32 width,
                         s32 height, s32 u, s32 v, s32 clut) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)u;
    (void)v;
    (void)clut;
    return prim + 1;
}

u8 *GameQueueShadedSpriteTrans(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                               s32 width, s32 height, s32 u, s32 v,
                               s32 clut, s32 intensity) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)u;
    (void)clut;
    s_choices[s_choiceCount++] = (ChoiceRecord){v, intensity};
    return prim + 1;
}

u8 *AddTilePrim(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                s32 width, s32 height,
                s32 r, s32 g, s32 b) {
    (void)ot;
    (void)y;
    (void)width;
    (void)height;
    (void)r;
    (void)g;
    (void)b;
    s_tileCount++;
    s_lastHighlightX = x;
    return prim + 1;
}

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
    g_RenderState.draw.packetCursor = s_packets;
    g_BgmVolumeSetting = 7;
    g_SfxVolumeSetting = 5;
    g_MonoOutput = 0;
    g_GameMode = OPTION_MODE_SOUND_MENU;
    s_screen.cursor = 0;
    g_PadPressed = 0;
    s_choiceCount = 0;
    s_applyCalls = 0;
    s_lastCue = 0;
    s_lastHighlightX = -1;
    s_tileCount = 0;
    s_volumeCalls = 0;
}

int main(void) {
    s32 cursor;

    Reset();
    s_screen.cursor = 3;
    s_screen.savedValue = 9;
    EnterSoundOptionMenuState(&s_screen);
    CHECK(s_screen.cursor == 0 && s_screen.savedValue == 0);

    Reset();
    UpdateSoundOptionMenuState(&s_screen);
    CHECK(s_choiceCount == 2);
    CHECK(s_choices[0].v == 0xC4 && s_choices[0].intensity == 0x7F);
    CHECK(s_choices[1].v == 0xD0 && s_choices[1].intensity == 0x20);
    CHECK(s_volumeCalls == 2 && s_volumeLevels[0] == 7 &&
          s_volumeLevels[1] == 5);
    CHECK(s_tileCount == 4);

    Reset();
    g_GameMode = OPTION_MODE_SOUND_EDIT;
    g_MonoOutput = 1;
    s_screen.cursor = 2;
    UpdateSoundSettingAdjustState(&s_screen);
    CHECK(s_choices[0].intensity == 0x20 &&
          s_choices[1].intensity == 0x7F);
    CHECK(s_tileCount == 5 && s_lastHighlightX == 0xA0);

    Reset();
    g_PadPressed = PAD_UP;
    UpdateSoundOptionMenuState(&s_screen);
    CHECK(s_screen.cursor == 3 && s_lastCue == 1);

    for (cursor = 0; cursor < 3; cursor++) {
        Reset();
        s_screen.cursor = cursor;
        g_PadPressed = PAD_CONFIRM;
        UpdateSoundOptionMenuState(&s_screen);
        CHECK(g_GameMode == OPTION_MODE_SOUND_EDIT && s_lastCue == 2);
        CHECK(s_screen.savedValue ==
              (cursor == 0 ? 7 : cursor == 1 ? 5 : 0));
    }

    Reset();
    s_screen.cursor = 3;
    g_PadPressed = PAD_CONFIRM;
    UpdateSoundOptionMenuState(&s_screen);
    CHECK(g_GameMode == OPTION_MODE_ROOT && s_lastCue == 2);

    Reset();
    g_PadPressed = PAD_CANCEL;
    UpdateSoundOptionMenuState(&s_screen);
    CHECK(g_GameMode == OPTION_MODE_ROOT && s_lastCue == 3);

    Reset();
    g_GameMode = OPTION_MODE_SOUND_EDIT;
    s_screen.cursor = 0;
    g_PadPressed = PAD_RIGHT;
    UpdateSoundSettingAdjustState(&s_screen);
    CHECK(g_BgmVolumeSetting == 8 && g_GameMode == OPTION_MODE_SOUND_EDIT);
    CHECK(s_lastCue == 1 && s_applyCalls == 1);

    Reset();
    g_GameMode = OPTION_MODE_SOUND_EDIT;
    s_screen.cursor = 1;
    s_screen.savedValue = 9;
    g_PadPressed = PAD_CANCEL;
    UpdateSoundSettingAdjustState(&s_screen);
    CHECK(g_SfxVolumeSetting == 9 && g_GameMode == OPTION_MODE_SOUND_MENU);
    CHECK(s_lastCue == 3 && s_applyCalls == 1);

    Reset();
    g_GameMode = OPTION_MODE_SOUND_EDIT;
    s_screen.cursor = 2;
    g_PadPressed = PAD_RIGHT | PAD_CONFIRM;
    UpdateSoundSettingAdjustState(&s_screen);
    CHECK(g_MonoOutput == 0 && g_GameMode == OPTION_MODE_SOUND_MENU);
    CHECK(s_lastCue == 2 && s_applyCalls == 1);

    Reset();
    g_GameMode = OPTION_MODE_SOUND_EDIT;
    s_screen.cursor = 0;
    g_BgmVolumeSetting = 15;
    g_PadPressed = PAD_RIGHT;
    UpdateSoundSettingAdjustState(&s_screen);
    CHECK(g_BgmVolumeSetting == 15 && s_lastCue == 0);

    Reset();
    g_GameMode = OPTION_MODE_SOUND_EDIT;
    s_screen.cursor = 99;
    g_PadPressed = PAD_RIGHT;
    UpdateSoundSettingAdjustState(&s_screen);
    CHECK(g_GameMode == OPTION_MODE_SOUND_MENU);
    CHECK(g_BgmVolumeSetting == 7 && g_SfxVolumeSetting == 5);
    CHECK(s_lastCue == 0 && s_applyCalls == 1);

    Reset();
    g_BgmVolumeSetting = INT_MIN;
    g_SfxVolumeSetting = INT_MAX;
    g_MonoOutput = INT_MIN;
    s_screen.cursor = INT_MAX;
    UpdateSoundOptionMenuState(&s_screen);
    CHECK(g_BgmVolumeSetting == 0 &&
          g_SfxVolumeSetting == AUDIO_SETTING_MAX);
    CHECK(g_MonoOutput == 1 && s_screen.cursor == 3);
    CHECK(s_lastCue == 0);

    Reset();
    g_GameMode = OPTION_MODE_SOUND_EDIT;
    s_screen.cursor = 0;
    s_screen.savedValue = INT_MAX;
    g_PadPressed = PAD_CANCEL;
    UpdateSoundSettingAdjustState(&s_screen);
    CHECK(g_BgmVolumeSetting == AUDIO_SETTING_MAX);
    CHECK(g_GameMode == OPTION_MODE_SOUND_MENU && s_applyCalls == 1);

    puts("sound option menu preserves output rendering and navigation");
    return 0;
}
