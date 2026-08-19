#include "common.h"
#include "game/game_input.h"
#include "game/memcard.h"
#include "game/audio.h"
#include "game/menu.h"
#include "game/state.h"
#include "psyq/gpu.h"
#include "game/render.h"
#include "game/game_context.h"
#include "game/memory_card_controller.h"


void DrawMemoryCardSaveRows(s32 flags, GameSaveHeaderRow *rows) {
    char text[16];
    s32 flags_reg = flags;
    s32 color = 0x7F;
    s32 width = 0x244;
    s32 height = 0xA0;
    char *text_ptr = text;
    s32 y = 0xD8;
    s32 row_bit = 1;
    GameSaveHeaderRowAddress row;
    GameSaveHeaderRowAddress end;

    row.pointer = rows;

    do {
        if (flags_reg % 2) {
            s32 i;

            sprintf(text, g_FmtSaveRow, row_bit);
            DrawLargeText(0x48, y, text, 0x7F, color, color, width, height);

            for (i = 0; i < row.pointer->fields.nameLength; i++) {
                text_ptr[i] = g_SaveNameCharset[row.pointer->fields.name[i]];
            }
            while (i < 7) {
                text_ptr[i++] = ' ';
            }
            snprintf(text + 6, sizeof(text) - 6, "%s", g_FmtSaveRowTail);
            DrawLargeText(0x68, y, text, 0x7F, color, color, width, height);
            DrawLargeText(0xB0, y, FormatSaveElapsedTime(text, row.pointer->fields.saveCounter), 0x7F, color, color, width, height);
        } else if (flags_reg & 0x10000) {
            sprintf(text, g_FmtSaveRow, row_bit);
            DrawLargeText(0x48, y, text, 0x7F, color, color, width, height);
            DrawLargeText(0x88, y, g_McSlotLabelError, 0x7F, color, color, width, height);
        } else if (g_McFreeBlocks == 0) {
            if (g_McMenuPage == 0) {
                sprintf(text, g_FmtSaveRowEmpty, row_bit);
                DrawLargeText(0x48, y, text, 0x7F, color, color, width, height);
            } else if (g_McMenuRowCursor == 0) {
                sprintf(text, g_FmtSaveRowEmpty, row_bit);
                DrawLargeText(0x48, y, text, 0x7F, color, color, width, height);
            } else {
                sprintf(text, g_FmtSaveRow, row_bit);
                DrawLargeText(0x48, y, text, 0x7F, color, color, width, height);
                DrawLargeText(0x90, y, g_McSlotLabelNoFile, 0x7F, color, color, width, height);
            }
        } else if (g_McMenuPage == 0) {
            sprintf(text, g_FmtSaveRowEmpty, row_bit);
            DrawLargeText(0x48, y, text, 0x7F, color, color, width, height);
        } else {
            char *slotLabel;

            sprintf(text, g_FmtSaveRow, row_bit);
            DrawLargeText(0x48, y, text, 0x7F, color, color, width, height);
            /* The retail link layout placed NEW FILE and NO FILE ten bytes
             * apart.  Name the two objects explicitly so the game does not
             * depend on unrelated globals remaining adjacent on a host ABI. */
            slotLabel = g_McMenuRowCursor == 0 ?
                g_McSlotLabels : g_McSlotLabelNoFile;
            DrawLargeText(0x90, y, slotLabel, 0x7F, color, color, width, height);
        }

        row_bit++;
        row.pointer++;
        y += 0x30;
        flags_reg >>= 1;
        end.pointer = rows + 3;
    } while (row.value < end.value);
}

void AdjustMenuSelectionHorizontal(s32 *value, s32 min, s32 max) {
    MemoryCardCursorResult result = MemoryCardMoveMenuRow(
        *value, min, max, g_GameInput.pressedRepeat);
    *value = result.value;
    if (result.moved) PlaySoundCue(1);
}

void SetMenuBinaryChoiceVertical(s32 *value) {
    MemoryCardCursorResult result = MemoryCardSetBinaryChoice(
        *value, g_GameInput.pressedRepeat);
    *value = result.value;
    if (result.moved) PlaySoundCue(1);
}

u16 PollMenuConfirmInput(void) {
    u16 *state = &g_GameInput.pressed;
    u16 value;

    value = *state & 0x860;
    if (value != 0) {
        PlaySoundCue(2);
    }

    return *state & 0x860;
}

u16 PollMenuBackInput(void) {
    u16 *state = &g_GameInput.pressed;
    u16 value;

    value = *state & 0x90;
    if (value != 0) {
        PlaySoundCue(3);
    }

    return *state & 0x90;
}


void DrawMenuFadeOverlay(s32 level) {
    DrawFullscreenFadeTile480(level, 0x40);
}

void StartMenuExitFade(void) {
    StopMemoryCardEvents();
    g_McFadeStep = 8;
}

void EnterMemoryCardMenu(void) {
    SetDispMask(0);
    SetupDisplay480(0, 0, 0);
    g_McMenuRowCount = 2;
    g_McMenuState = MC_MENU_NO_CARD;
    g_SceneTimer = 0;
    g_McMenuPage = 0;
    g_McMenuRowCursor = 0;
    g_McMenuSubState = 1;
    g_McFromLoadMenu = 0;
    StartMemoryCardEvents();
    g_McFadeStep = -8;
    g_McFadeLevel = 0xFF;
    GameSceneSet(SCENE_MEMORY_CARD);
}
