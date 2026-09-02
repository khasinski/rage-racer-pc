#include <string.h>

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

enum {
    RECORD_ENTRY_OPAQUE_FADE = 0x100,
    RECORD_ENTRY_FRAME_SYNC_THRESHOLD = 0x80,
    RECORD_ENTRY_SCENE_ID = 0x15,
    RECORD_ENTRY_MUSIC_TRACK = 0xE,
    RECORD_ENTRY_FADE_IN_STEP = 8,
    RECORD_ENTRY_FADE_OUT_STEP = 2,
    RECORD_ENTRY_PANEL_WIDTH = 0x140,
    RECORD_ENTRY_PANEL_STEP = 8,
    DEFAULT_NAME_ENTRY_CHARACTER = 0xB,
    RECORD_ENTRY_MUSIC_FADE = 0x78,
    BGM_SELECT_SCENE_ID = 6,
};

static void InsertRaceRecords(void) {
    FastestLap fastestLap;
    s32 lapCount;
    s32 course;

    lapCount = CourseLapCount(g_CourseIndex);
    fastestLap = FindFastestLap(
        g_PlayerCar.lapTimes.table.milliseconds, lapCount);
    g_BestLapIndex = fastestLap.index;

    course = SeriesCourseIndex();
    g_RankingInsertRow = InsertRaceRecord(
        g_RankingRecords[g_GrandPrixSeries][course], fastestLap.time,
        g_PlayerCarIndex,
        g_RankingNameCodes);
    g_TimeRecordInsertRow = InsertRaceRecord(
        g_TimeRecords[g_GrandPrixSeries][course], g_RaceTotalTime,
        g_PlayerCarIndex, g_TimeRecordNameCodes);
}

void EnterRecordEntry(void) {
    g_SceneTimer = RECORD_ENTRY_OPAQUE_FADE;
    g_FrameSyncThreshold = RECORD_ENTRY_FRAME_SYNC_THRESHOLD;
    g_RecordEntryState = RECORD_ENTRY_STATE_FADE_IN;
    g_SceneId = RECORD_ENTRY_SCENE_ID;
    InsertRaceRecords();
}

static s32 RecordWasInserted(s32 row) {
    return row < RECORD_TABLE_LENGTH;
}

static s32 AnyRecordWasInserted(void) {
    return RecordWasInserted(g_RankingInsertRow) ||
           RecordWasInserted(g_TimeRecordInsertRow);
}

static void UpdateRecordEntryFadeIn(void) {
    g_SceneTimer -= RECORD_ENTRY_FADE_IN_STEP;
    DrawFullscreenFadeTile(g_SceneTimer, 0x49);
    if (g_SceneTimer == 0) {
        if (AnyRecordWasInserted()) {
            RequestCdTrack(RECORD_ENTRY_MUSIC_TRACK);
            StartCdAudio();
        }
        if (RecordWasInserted(g_RankingInsertRow)) {
            g_NameEntryChar = DEFAULT_NAME_ENTRY_CHARACTER;
            g_NameEntryCursor = 0;
            g_RecordEntryState = RECORD_ENTRY_STATE_EDIT_LAP_NAME;
        } else {
            g_RecordEntryState = RECORD_ENTRY_STATE_WAIT_AFTER_LAP_NAME;
        }
    }
    DrawRankingPanel(0);
}

static void UpdateLapRecordName(void) {
    s32 course = SeriesCourseIndex();

    if (UpdateRecordNameEntry(g_RankingNameCodes)) {
        g_RecordEntryState = RECORD_ENTRY_STATE_WAIT_AFTER_LAP_NAME;
        if (RecordWasInserted(g_TimeRecordInsertRow)) {
            memcpy(g_TimeRecordNameCodes, g_RankingNameCodes,
                   RECORD_NAME_LENGTH);
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
}

static void UpdateAfterLapRecord(void) {
    if (g_PadPressed & PAD_CONFIRM) {
        g_RecordEntryState = RECORD_ENTRY_STATE_SWITCH_TO_RACE_RECORD;
        g_RecordPanelSlide = 0;
    }
    DrawRankingPanel(0);
}

static void UpdateRecordPanelSwitch(void) {
    g_RecordPanelSlide -= RECORD_ENTRY_PANEL_STEP;
    DrawRankingPanel(g_RecordPanelSlide);
    DrawTimeRecordPanel(g_RecordPanelSlide + RECORD_ENTRY_PANEL_WIDTH);
    if (g_RecordPanelSlide <= -RECORD_ENTRY_PANEL_WIDTH) {
        if (RecordWasInserted(g_TimeRecordInsertRow)) {
            g_NameEntryCursor = 0;
            g_RecordEntryState = RECORD_ENTRY_STATE_EDIT_RACE_NAME;
            g_NameEntryChar = g_TimeRecordNameCodes[0];
        } else {
            g_RecordEntryState = RECORD_ENTRY_STATE_WAIT_TO_FINISH;
        }
    }
}

static void UpdateRaceRecordName(void) {
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
}

static void UpdateBeforeRecordEntryExit(void) {
    if (g_PadPressed & PAD_CONFIRM) {
        if (AnyRecordWasInserted()) {
            StartCdVolumeFade(RECORD_ENTRY_MUSIC_FADE);
            StartCdAudio();
        }
        g_RecordEntryState = RECORD_ENTRY_STATE_FADE_OUT;
        g_RecordPanelSlide = 0;
    }
    DrawTimeRecordPanel(0);
}

static void UpdateRecordEntryFadeOut(void) {
    g_SceneTimer += RECORD_ENTRY_FADE_OUT_STEP;
    DrawFullscreenFadeTile(g_SceneTimer, 0x49);
    if ((u32)g_SceneTimer >= RECORD_ENTRY_OPAQUE_FADE) {
        RequestSelectBgmAssets();
        g_SceneId = BGM_SELECT_SCENE_ID;
    }
    DrawTimeRecordPanel(0);
}

void UpdateRecordEntry(void) {
    g_AnimTimer++;

    switch (g_RecordEntryState) {
    case RECORD_ENTRY_STATE_INVALID:
        break;
    case RECORD_ENTRY_STATE_FADE_IN:
        UpdateRecordEntryFadeIn();
        break;
    case RECORD_ENTRY_STATE_EDIT_LAP_NAME:
        UpdateLapRecordName();
        break;
    case RECORD_ENTRY_STATE_WAIT_AFTER_LAP_NAME:
        UpdateAfterLapRecord();
        break;
    case RECORD_ENTRY_STATE_SWITCH_TO_RACE_RECORD:
        UpdateRecordPanelSwitch();
        break;
    case RECORD_ENTRY_STATE_EDIT_RACE_NAME:
        UpdateRaceRecordName();
        break;
    case RECORD_ENTRY_STATE_WAIT_TO_FINISH:
        UpdateBeforeRecordEntryExit();
        break;
    case RECORD_ENTRY_STATE_FADE_OUT:
        UpdateRecordEntryFadeOut();
        break;
    }

    DrawCourseIntro();
}
