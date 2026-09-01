#include <stdio.h>
#include "game/asset.h"
#include "game/prim.h"
#include "game/audio.h"
#include "game/player_car_internal.h"
#include "game/cd.h"
#include "game/race.h"
#include "game/records_internal.h"
#include "game/race_internal.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/screens.h"
#include "game/state.h"
#include "psyq/cd.h"

void DrawRankingPanel(s32 slideX) {
    char text[56];
    s32 lapCount;
    s32 row;
    s32 course = SeriesCourseIndex();

    DrawProportionalText(slideX + 0x10, 0x4C, g_CaptionLapTime2, 0x7852);
    text[1] = 0x2F;
    lapCount = g_CourseIndex == 3 ? 6 : 3;
    for (row = 0; row < lapCount; row++) {
        s32 column = row % 2;
        s32 x = slideX + 0x14 + (row / 2) * 0x60;
        s32 y = 0x58 + column * 8;
        s32 color = g_BestLapIndex == row ? 0x780F : 0x78CC;

        text[0] = row + 0x31;
        FormatLapTime(&text[2],
                      g_PlayerCar.lapTimes.table.milliseconds[row]);
        DrawText8x8(x, y, text, color);
    }

    DrawProportionalText(slideX + 0x10, 0x6C, g_CaptionRanking2, 0x7812);
    for (row = 0; row < 5; row++) {
        const RaceRecord *record =
            &g_RankingRecords[g_GrandPrixSeries][course][row];
        s32 carIndex = record->carIndex;
        s32 color = g_RankingInsertRow == row ? 0x780F : 0x78CC;
        s32 y = 0x78 + row * 0x14;

        text[0] = g_PlaceSuffixNames[row][0];
        text[1] = g_PlaceSuffixNames[row][1];
        text[2] = g_PlaceSuffixNames[row][2];
        text[3] = 0x2F;
        FormatLapTime(&text[4], record->raceTime);
        sprintf(&text[0xC], g_FmtRecordName, record,
                g_CarClassNames[carIndex]);
        DrawText8x8(slideX + 0x14, y, text, color);

        sprintf(text, g_FmtCarName, g_CarNames[carIndex]);
        DrawText8x8(slideX + 0x2C, y + 0xA, text, color);
    }
}

void DrawTimeRecordPanel(s32 slideX) {
    char text[48];
    s32 course = SeriesCourseIndex();
    s32 row;

    DrawProportionalText(slideX + 0x10, 0x4C, g_CaptionTotalTime2, 0x7852);

    text[0] = 0x54;
    text[1] = 0x2F;
    FormatLapTime(&text[2], g_RaceTotalTime);
    DrawText8x8(slideX + 0x14, 0x58, text, 0x78CC);

    DrawProportionalText(slideX + 0x10, 0x6C, g_CaptionRanking2, 0x7812);

    for (row = 0; row < 5; row++) {
        const RaceRecord *record =
            &g_TimeRecords[g_GrandPrixSeries][course][row];
        s32 carIndex = record->carIndex;
        s32 color = g_TimeRecordInsertRow == row ? 0x780F : 0x78CC;
        s32 y = 0x78 + row * 0x14;

        text[0] = g_PlaceSuffixNames[row][0];
        text[1] = g_PlaceSuffixNames[row][1];
        text[2] = g_PlaceSuffixNames[row][2];
        text[3] = 0x2F;
        FormatLapTime(&text[4], record->raceTime);
        sprintf(&text[0xC], g_FmtRecordName, record,
                g_CarClassNames[carIndex]);
        DrawText8x8(slideX + 0x14, y, text, color);

        sprintf(text, g_FmtCarName, g_CarNames[carIndex]);
        DrawText8x8(slideX + 0x2C, y + 0xA, text, color);
    }
}

void DrawNameEntryCursor(s32 charIndex, s32 row) {
    u8 **cursorSlot;

    if (g_AnimTimer & 8) {
        cursorSlot = &RENDER_PRIM_CURSOR_AS(u8);
        *cursorSlot = AddTilePrim(
            GamePrimaryOrderingTable(0),
            *cursorSlot,
            (charIndex * 8) + 0x7C,
            ((row * 5) << 2) + 0x7E,
            9,
            2,
            0xC0,
            0x48,
            0x48);
    }

}

void InsertRaceRecords(void) {
    s32 lapCount;
    s32 i;
    s32 bestLapTime;
    s32 course;

    lapCount = g_CourseIndex == 3 ? 6 : 3;
    bestLapTime = 0x927C0;
    for (i = 0; i < lapCount; i++) {
        s32 lapTime = g_PlayerCar.lapTimes.table.milliseconds[i];

        if (lapTime < bestLapTime) {
            bestLapTime = lapTime;
            g_BestLapIndex = i;
        }
    }

    course = SeriesCourseIndex();
    g_RankingInsertRow = InsertRaceRecord(
        g_RankingRecords[g_GrandPrixSeries][course], bestLapTime,
        g_PlayerCarIndex,
        g_RankingNameCodes);
    g_TimeRecordInsertRow = InsertRaceRecord(
        g_TimeRecords[g_GrandPrixSeries][course], g_RaceTotalTime,
        g_PlayerCarIndex, g_TimeRecordNameCodes);
}

void EnterRecordEntry(void) {
    g_SceneTimer = 0x100;
    g_FrameSyncThreshold = 0x80;
    g_RecordEntryState = RECORD_ENTRY_STATE_FADE_IN;
    g_SceneId = 0x15;
    InsertRaceRecords();
}

enum {
    RECORD_NAME_LENGTH = 6,
    NAME_ENTRY_CHARACTER_COUNT = 42,
};

static void WriteRecordName(RaceRecord *record, const u8 *nameCodes) {
    s32 character;

    for (character = 0; character < RECORD_NAME_LENGTH; character++) {
        record->driverName[character] =
            g_NameEntryCharset[nameCodes[character]];
    }
}

/* Updates one editable character and returns true once all six are accepted. */
static s32 UpdateNameEntryInput(u8 *nameCodes) {
    s32 previousCharacter = g_NameEntryChar;
    u16 buttons;

    if (g_PadPressedRepeat & PAD_LEFT) {
        g_NameEntryChar--;
    } else if (g_PadPressedRepeat & PAD_RIGHT) {
        g_NameEntryChar++;
    }
    g_NameEntryChar =
        (g_NameEntryChar + NAME_ENTRY_CHARACTER_COUNT) %
        NAME_ENTRY_CHARACTER_COUNT;
    if (previousCharacter != g_NameEntryChar) {
        PlaySoundCue(1);
    }

    nameCodes[g_NameEntryCursor] = g_NameEntryChar;
    buttons = g_PadPressed;
    if (buttons & 0x860) {
        PlaySoundCue(2);
        g_NameEntryCursor++;
        if (g_NameEntryCursor == RECORD_NAME_LENGTH) {
            return 1;
        }
        g_NameEntryChar = nameCodes[g_NameEntryCursor];
    } else if ((buttons & 0x90) && g_NameEntryCursor > 0) {
        PlaySoundCue(3);
        g_NameEntryCursor--;
        g_NameEntryChar = nameCodes[g_NameEntryCursor];
    }
    return 0;
}

void UpdateRecordEntry(void) {
    s32 i;

    g_AnimTimer++;

    switch (g_RecordEntryState) {
    case RECORD_ENTRY_STATE_INVALID:
        break;
    case RECORD_ENTRY_STATE_FADE_IN:
        g_SceneTimer -= 8;
        DrawFullscreenFadeTile(g_SceneTimer, 0x49);
        if (g_SceneTimer == 0) {
            if (g_RankingInsertRow < 5 || g_TimeRecordInsertRow < 5) {
                RequestCdTrack(0xE);
                StartCdAudio();
            }
            if (g_RankingInsertRow < 5) {
                g_NameEntryChar = 0xB;
                g_NameEntryCursor = 0;
                g_RecordEntryState = RECORD_ENTRY_STATE_EDIT_LAP_NAME;
            } else {
                g_RecordEntryState = RECORD_ENTRY_STATE_WAIT_AFTER_LAP_NAME;
            }
        }
        DrawRankingPanel(0);
        break;

    case RECORD_ENTRY_STATE_EDIT_LAP_NAME: {
        s32 course = SeriesCourseIndex();

        if (UpdateNameEntryInput(g_RankingNameCodes)) {
            g_RecordEntryState = RECORD_ENTRY_STATE_WAIT_AFTER_LAP_NAME;
            if (g_TimeRecordInsertRow < 5) {
                for (i = 0; i < RECORD_NAME_LENGTH; i++) {
                    g_TimeRecordNameCodes[i] = g_RankingNameCodes[i];
                }
                WriteRecordName(
                    &g_TimeRecords[g_GrandPrixSeries][course]
                                  [g_TimeRecordInsertRow],
                    g_TimeRecordNameCodes);
            }
        }

        if (g_RecordEntryState == RECORD_ENTRY_STATE_EDIT_LAP_NAME) {
            DrawNameEntryCursor(g_NameEntryCursor, g_RankingInsertRow);
        }
        WriteRecordName(
            &g_RankingRecords[g_GrandPrixSeries][course][g_RankingInsertRow],
            g_RankingNameCodes);
        DrawRankingPanel(0);
        break;
    }

    case RECORD_ENTRY_STATE_WAIT_AFTER_LAP_NAME:
        if (g_PadPressed & PAD_CONFIRM) {
            g_RecordEntryState = RECORD_ENTRY_STATE_SWITCH_TO_RACE_RECORD;
            g_RecordPanelSlide = 0;
        }
        DrawRankingPanel(0);
        break;

    case RECORD_ENTRY_STATE_SWITCH_TO_RACE_RECORD:
        g_RecordPanelSlide -= 8;
        DrawRankingPanel(g_RecordPanelSlide);
        DrawTimeRecordPanel(g_RecordPanelSlide + 0x140);
        if (g_RecordPanelSlide < -0x13F) {
            if (g_TimeRecordInsertRow < 5) {
                g_NameEntryCursor = 0;
                g_RecordEntryState = RECORD_ENTRY_STATE_EDIT_RACE_NAME;
                g_NameEntryChar = g_TimeRecordNameCodes[0];
            } else {
                g_RecordEntryState = RECORD_ENTRY_STATE_WAIT_TO_FINISH;
            }
        }
        break;

    case RECORD_ENTRY_STATE_EDIT_RACE_NAME: {
        s32 course = SeriesCourseIndex();

        if (UpdateNameEntryInput(g_TimeRecordNameCodes)) {
            g_RecordEntryState = RECORD_ENTRY_STATE_WAIT_TO_FINISH;
        }

        if (g_RecordEntryState == RECORD_ENTRY_STATE_EDIT_RACE_NAME) {
            DrawNameEntryCursor(g_NameEntryCursor, g_TimeRecordInsertRow);
        }
        WriteRecordName(
            &g_TimeRecords[g_GrandPrixSeries][course][g_TimeRecordInsertRow],
            g_TimeRecordNameCodes);
        DrawTimeRecordPanel(0);
        break;
    }

    case RECORD_ENTRY_STATE_WAIT_TO_FINISH:
        if (g_PadPressed & PAD_CONFIRM) {
            if (g_RankingInsertRow < 5 || g_TimeRecordInsertRow < 5) {
                StartCdVolumeFade(0x78);
                StartCdAudio();
            }
            g_RecordEntryState = RECORD_ENTRY_STATE_FADE_OUT;
            g_RecordPanelSlide = 0;
        }
        DrawTimeRecordPanel(0);
        break;

    case RECORD_ENTRY_STATE_FADE_OUT:
        g_SceneTimer += 2;
        DrawFullscreenFadeTile(g_SceneTimer, 0x49);
        if ((u32)g_SceneTimer >= 0x100) {
            RequestSelectBgmAssets();
            g_SceneId = 6;
        }
        DrawTimeRecordPanel(0);
        break;
    }

    DrawCourseIntro();
}

static void StopFmvCdPlayback(void) {
    CdSync(0, 0);
    CdControl(CD_DRIVE_PAUSE, 0, 0);
}

void ReturnFromClassFmv(void) {
    StopFmvCdPlayback();
    g_SceneId = 6;
    RequestSelectBgmAssets();
}

void ReturnFromEndingFmv(void) {
    StopFmvCdPlayback();
    SetDispMask(0);
    SetupDisplay240(0, 0, 0);
    g_FrameSyncThreshold = 0x80;
    g_FadeStep = 4;
    g_FadeLevel = 0;
    g_SceneId = 0x22;
    g_SceneTimer = 0;
}
