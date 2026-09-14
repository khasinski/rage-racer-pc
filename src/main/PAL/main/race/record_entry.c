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
#include "game/scene.h"
#include "game/scene_runtime.h"
#include "game/state.h"

enum {
    RECORD_ENTRY_OPAQUE_FADE = 0x100,
    RECORD_ENTRY_FRAME_SYNC_THRESHOLD = 0x80,
    RECORD_ENTRY_MUSIC_TRACK = 0xE,
    RECORD_ENTRY_FADE_IN_STEP = 8,
    RECORD_ENTRY_FADE_OUT_STEP = 2,
    RECORD_ENTRY_PANEL_WIDTH = 0x140,
    RECORD_ENTRY_PANEL_STEP = 8,
    DEFAULT_NAME_ENTRY_CHARACTER = 0xB,
    RECORD_ENTRY_MUSIC_FADE = 0x78,
};

static void InsertRaceRecords(RecordEntry *state) {
    FastestLap fastestLap;
    s32 lapCount;
    s32 course;

    lapCount = CourseLapCount(g_CourseIndex);
    fastestLap = FindFastestLap(
        g_PlayerCar.lapTimes.table.milliseconds, lapCount);
    state->bestLap = fastestLap.index;

    course = SeriesCourseIndex();
    if ((u32)g_GrandPrixSeries >= RECORD_SERIES_COUNT) {
        state->rankingRow = RECORD_TABLE_LENGTH;
        state->timeRow = RECORD_TABLE_LENGTH;
        return;
    }
    state->rankingRow = InsertRaceRecord(
        g_RankingRecords[g_GrandPrixSeries][course], fastestLap.time,
        g_PlayerCarIndex,
        g_RankingNameCodes);
    state->timeRow = InsertRaceRecord(
        g_TimeRecords[g_GrandPrixSeries][course], g_RaceTotalTime,
        g_PlayerCarIndex, g_TimeRecordNameCodes);
}

void EnterRecordEntry(void) {
    RecordEntry *state = SceneRuntimeRecordEntry();

    g_SceneTimer = RECORD_ENTRY_OPAQUE_FADE;
    g_FrameSyncThreshold = RECORD_ENTRY_FRAME_SYNC_THRESHOLD;
    state->step = RECORD_ENTRY_STATE_FADE_IN;
    g_SceneId = GAME_SCENE_RECORD_ENTRY;
    InsertRaceRecords(state);
}

static s32 RecordWasInserted(s32 row) {
    return (u32)g_GrandPrixSeries < RECORD_SERIES_COUNT &&
           (u32)row < RECORD_TABLE_LENGTH;
}

static s32 AnyRecordWasInserted(const RecordEntry *state) {
    return RecordWasInserted(state->rankingRow) ||
           RecordWasInserted(state->timeRow);
}

static void UpdateRecordEntryFadeIn(RecordEntry *state) {
    if (g_SceneTimer > RECORD_ENTRY_OPAQUE_FADE) {
        g_SceneTimer = RECORD_ENTRY_OPAQUE_FADE;
    }
    g_SceneTimer = g_SceneTimer <= RECORD_ENTRY_FADE_IN_STEP
                       ? 0
                       : g_SceneTimer - RECORD_ENTRY_FADE_IN_STEP;
    DrawFullscreenFadeTile(g_SceneTimer, 0x49);
    if (g_SceneTimer == 0) {
        if (AnyRecordWasInserted(state)) {
            RequestCdTrack(RECORD_ENTRY_MUSIC_TRACK);
            StartCdAudio();
        }
        if (RecordWasInserted(state->rankingRow)) {
            state->nameCharacter = DEFAULT_NAME_ENTRY_CHARACTER;
            state->nameCursor = 0;
            state->step = RECORD_ENTRY_STATE_EDIT_LAP_NAME;
        } else {
            state->step = RECORD_ENTRY_STATE_WAIT_AFTER_LAP_NAME;
        }
    }
    DrawRankingPanel(state, 0);
}

static void UpdateLapRecordName(RecordEntry *state) {
    s32 course = SeriesCourseIndex();

    if (!RecordWasInserted(state->rankingRow)) {
        state->step = RECORD_ENTRY_STATE_WAIT_AFTER_LAP_NAME;
        DrawRankingPanel(state, 0);
        return;
    }

    if (UpdateRecordNameEntry(state, g_RankingNameCodes)) {
        state->step = RECORD_ENTRY_STATE_WAIT_AFTER_LAP_NAME;
        if (RecordWasInserted(state->timeRow)) {
            memcpy(g_TimeRecordNameCodes, g_RankingNameCodes,
                   RECORD_NAME_LENGTH);
            WriteRecordDriverName(
                &g_TimeRecords[g_GrandPrixSeries][course]
                              [state->timeRow],
                g_TimeRecordNameCodes);
        }
    }

    if (state->step == RECORD_ENTRY_STATE_EDIT_LAP_NAME) {
        DrawNameEntryCursor(state->nameCursor, state->rankingRow);
    }
    WriteRecordDriverName(
        &g_RankingRecords[g_GrandPrixSeries][course][state->rankingRow],
        g_RankingNameCodes);
    DrawRankingPanel(state, 0);
}

static void UpdateAfterLapRecord(RecordEntry *state) {
    if (g_PadPressed & PAD_CONFIRM) {
        state->step = RECORD_ENTRY_STATE_SWITCH_TO_RACE_RECORD;
        state->panelSlide = 0;
    }
    DrawRankingPanel(state, 0);
}

static void UpdateRecordPanelSwitch(RecordEntry *state) {
    if (state->panelSlide > 0) {
        state->panelSlide = 0;
    }
    state->panelSlide =
        state->panelSlide <= -RECORD_ENTRY_PANEL_WIDTH + RECORD_ENTRY_PANEL_STEP
            ? -RECORD_ENTRY_PANEL_WIDTH
            : state->panelSlide - RECORD_ENTRY_PANEL_STEP;
    DrawRankingPanel(state, state->panelSlide);
    DrawTimeRecordPanel(state, state->panelSlide + RECORD_ENTRY_PANEL_WIDTH);
    if (state->panelSlide <= -RECORD_ENTRY_PANEL_WIDTH) {
        if (RecordWasInserted(state->timeRow)) {
            state->nameCursor = 0;
            state->step = RECORD_ENTRY_STATE_EDIT_RACE_NAME;
            state->nameCharacter = g_TimeRecordNameCodes[0];
        } else {
            state->step = RECORD_ENTRY_STATE_WAIT_TO_FINISH;
        }
    }
}

static void UpdateRaceRecordName(RecordEntry *state) {
    s32 course = SeriesCourseIndex();

    if (!RecordWasInserted(state->timeRow)) {
        state->step = RECORD_ENTRY_STATE_WAIT_TO_FINISH;
        DrawTimeRecordPanel(state, 0);
        return;
    }

    if (UpdateRecordNameEntry(state, g_TimeRecordNameCodes)) {
        state->step = RECORD_ENTRY_STATE_WAIT_TO_FINISH;
    }

    if (state->step == RECORD_ENTRY_STATE_EDIT_RACE_NAME) {
        DrawNameEntryCursor(state->nameCursor, state->timeRow);
    }
    WriteRecordDriverName(
        &g_TimeRecords[g_GrandPrixSeries][course][state->timeRow],
        g_TimeRecordNameCodes);
    DrawTimeRecordPanel(state, 0);
}

static void UpdateBeforeRecordEntryExit(RecordEntry *state) {
    if (g_PadPressed & PAD_CONFIRM) {
        if (AnyRecordWasInserted(state)) {
            StartCdVolumeFade(RECORD_ENTRY_MUSIC_FADE);
            StartCdAudio();
        }
        state->step = RECORD_ENTRY_STATE_FADE_OUT;
        state->panelSlide = 0;
    }
    DrawTimeRecordPanel(state, 0);
}

static void UpdateRecordEntryFadeOut(RecordEntry *state) {
    if (g_SceneTimer < 0) {
        g_SceneTimer = 0;
    }
    g_SceneTimer =
        g_SceneTimer >= RECORD_ENTRY_OPAQUE_FADE - RECORD_ENTRY_FADE_OUT_STEP
            ? RECORD_ENTRY_OPAQUE_FADE
            : g_SceneTimer + RECORD_ENTRY_FADE_OUT_STEP;
    DrawFullscreenFadeTile(g_SceneTimer, 0x49);
    if ((u32)g_SceneTimer >= RECORD_ENTRY_OPAQUE_FADE) {
        RequestSelectBgmAssets();
        g_SceneId = GAME_SCENE_INIT_MENU;
    }
    DrawTimeRecordPanel(state, 0);
}

void UpdateRecordEntry(void) {
    RecordEntry *state = SceneRuntimeRecordEntry();

    g_AnimTimer = (s32)((u32)g_AnimTimer + 1u);

    switch (state->step) {
    case RECORD_ENTRY_STATE_INVALID:
        break;
    case RECORD_ENTRY_STATE_FADE_IN:
        UpdateRecordEntryFadeIn(state);
        break;
    case RECORD_ENTRY_STATE_EDIT_LAP_NAME:
        UpdateLapRecordName(state);
        break;
    case RECORD_ENTRY_STATE_WAIT_AFTER_LAP_NAME:
        UpdateAfterLapRecord(state);
        break;
    case RECORD_ENTRY_STATE_SWITCH_TO_RACE_RECORD:
        UpdateRecordPanelSwitch(state);
        break;
    case RECORD_ENTRY_STATE_EDIT_RACE_NAME:
        UpdateRaceRecordName(state);
        break;
    case RECORD_ENTRY_STATE_WAIT_TO_FINISH:
        UpdateBeforeRecordEntryExit(state);
        break;
    case RECORD_ENTRY_STATE_FADE_OUT:
        UpdateRecordEntryFadeOut(state);
        break;
    }

    DrawCourseIntro();
}
