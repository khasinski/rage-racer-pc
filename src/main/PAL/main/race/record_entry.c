#include "game/asset.h"
#include "game/audio.h"
#include "game/cd.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/records_internal.h"
#include "game/race_internal.h"
#include "game/render.h"
#include "game/screens.h"
#include "game/state.h"

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

        if (UpdateRecordNameEntry(g_RankingNameCodes)) {
            g_RecordEntryState = RECORD_ENTRY_STATE_WAIT_AFTER_LAP_NAME;
            if (g_TimeRecordInsertRow < 5) {
                for (i = 0; i < RECORD_NAME_LENGTH; i++) {
                    g_TimeRecordNameCodes[i] = g_RankingNameCodes[i];
                }
                WriteRecordDriverName(
                    &g_TimeRecords[g_GrandPrixSeries][course]
                                  [g_TimeRecordInsertRow],
                    g_TimeRecordNameCodes);
            }
        }

        if (g_RecordEntryState == RECORD_ENTRY_STATE_EDIT_LAP_NAME) {
            DrawNameEntryCursor(g_NameEntryCursor, g_RankingInsertRow);
        }
        WriteRecordDriverName(
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

        if (UpdateRecordNameEntry(g_TimeRecordNameCodes)) {
            g_RecordEntryState = RECORD_ENTRY_STATE_WAIT_TO_FINISH;
        }

        if (g_RecordEntryState == RECORD_ENTRY_STATE_EDIT_RACE_NAME) {
            DrawNameEntryCursor(g_NameEntryCursor, g_TimeRecordInsertRow);
        }
        WriteRecordDriverName(
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
