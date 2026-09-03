#include <stdio.h>
#include <string.h>

#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/records_internal.h"
#include "game/state.h"

s32 g_AnimTimer;
s32 g_BestLapIndex;
s32 g_CourseIndex;
s32 g_FrameSyncThreshold;
s16 g_GrandPrixSeries;
s32 g_NameEntryChar;
s32 g_NameEntryCursor;
u16 g_PadPressed;
PlayerCarRuntime g_PlayerCar;
s32 g_PlayerCarIndex;
s32 g_RaceTotalTime;
RaceRecord g_RankingRecords[2][4][5];
s32 g_RankingInsertRow;
u8 g_RankingNameCodes[RECORD_NAME_LENGTH];
RecordEntryState g_RecordEntryState;
s32 g_RecordPanelSlide;
s32 g_SceneId;
s32 g_SceneTimer;
RaceRecord g_TimeRecords[2][4][5];
s32 g_TimeRecordInsertRow;
u8 g_TimeRecordNameCodes[RECORD_NAME_LENGTH];

static s32 s_assetRequests;
static s32 s_cdStarts;
static s32 s_cdTrack;
static s32 s_cdFade;
static s32 s_courseIntroDraws;
static s32 s_cursorDraws;
static s32 s_fadeDraws;
static s32 s_nameEntryDone;
static s32 s_rankingDraws;
static s32 s_timeDraws;
static s32 s_writeCount;

FastestLap FindFastestLap(const s32 *lapTimes, s32 lapCount) {
    FastestLap result = {0, lapTimes[0]};
    (void)lapCount;
    return result;
}

s32 InsertRaceRecord(RaceRecord records[RECORD_TABLE_LENGTH], s32 raceTime,
                     s16 carIndex, u8 nameCodes[RECORD_NAME_LENGTH]) {
    (void)records;
    (void)raceTime;
    (void)carIndex;
    (void)nameCodes;
    return RECORD_TABLE_LENGTH;
}

void WriteRecordDriverName(RaceRecord *record, const u8 *nameCodes) {
    (void)record;
    (void)nameCodes;
    s_writeCount++;
}

s32 UpdateRecordNameEntry(u8 *nameCodes) {
    (void)nameCodes;
    return s_nameEntryDone;
}

void DrawNameEntryCursor(s32 charIndex, s32 row) {
    (void)charIndex;
    (void)row;
    s_cursorDraws++;
}

void DrawRankingPanel(s32 slideX) {
    (void)slideX;
    s_rankingDraws++;
}

void DrawTimeRecordPanel(s32 slideX) {
    (void)slideX;
    s_timeDraws++;
}

void DrawCourseIntro(void) { s_courseIntroDraws++; }

void DrawFullscreenFadeTile(s32 color, s32 tpage) {
    (void)color;
    (void)tpage;
    s_fadeDraws++;
}

void RequestCdTrack(s32 track) { s_cdTrack = track; }
void StartCdAudio(void) { s_cdStarts++; }
void StartCdVolumeFade(s32 frames) { s_cdFade = frames; }
s32 RequestSelectBgmAssets(void) {
    s_assetRequests++;
    return 0;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void Reset(void) {
    memset(g_RankingNameCodes, 0, sizeof(g_RankingNameCodes));
    memset(g_TimeRecordNameCodes, 0, sizeof(g_TimeRecordNameCodes));
    g_AnimTimer = 0;
    g_GrandPrixSeries = 0;
    g_NameEntryChar = 0;
    g_NameEntryCursor = 4;
    g_PadPressed = 0;
    g_RankingInsertRow = RECORD_TABLE_LENGTH;
    g_RecordPanelSlide = 123;
    g_SceneId = 0;
    g_SceneTimer = 8;
    g_TimeRecordInsertRow = RECORD_TABLE_LENGTH;
    s_assetRequests = 0;
    s_cdStarts = 0;
    s_cdTrack = -1;
    s_cdFade = 0;
    s_courseIntroDraws = 0;
    s_cursorDraws = 0;
    s_fadeDraws = 0;
    s_nameEntryDone = 0;
    s_rankingDraws = 0;
    s_timeDraws = 0;
    s_writeCount = 0;
}

int main(void) {
    Reset();
    g_RecordEntryState = RECORD_ENTRY_STATE_FADE_IN;
    UpdateRecordEntry();
    CHECK(g_RecordEntryState == RECORD_ENTRY_STATE_WAIT_AFTER_LAP_NAME);
    CHECK(g_SceneTimer == 0 && s_fadeDraws == 1);
    CHECK(s_cdStarts == 0 && s_cdTrack == -1);

    Reset();
    g_RankingInsertRow = -1;
    g_RecordEntryState = RECORD_ENTRY_STATE_FADE_IN;
    UpdateRecordEntry();
    CHECK(g_RecordEntryState == RECORD_ENTRY_STATE_WAIT_AFTER_LAP_NAME);
    CHECK(s_cdStarts == 0 && s_cdTrack == -1);

    Reset();
    g_RankingInsertRow = 0;
    g_TimeRecordInsertRow = 1;
    g_RecordEntryState = RECORD_ENTRY_STATE_FADE_IN;
    UpdateRecordEntry();
    CHECK(g_RecordEntryState == RECORD_ENTRY_STATE_EDIT_LAP_NAME);
    CHECK(g_NameEntryCursor == 0 && g_NameEntryChar == 0xB);
    CHECK(s_cdTrack == 0xE && s_cdStarts == 1);

    g_RankingNameCodes[0] = 7;
    UpdateRecordEntry();
    CHECK(s_cursorDraws == 1 && s_writeCount == 1);
    s_nameEntryDone = 1;
    UpdateRecordEntry();
    CHECK(g_RecordEntryState == RECORD_ENTRY_STATE_WAIT_AFTER_LAP_NAME);
    CHECK(memcmp(g_TimeRecordNameCodes, g_RankingNameCodes,
                 RECORD_NAME_LENGTH) == 0);
    CHECK(s_writeCount == 3);

    g_PadPressed = PAD_CONFIRM;
    UpdateRecordEntry();
    CHECK(g_RecordEntryState == RECORD_ENTRY_STATE_SWITCH_TO_RACE_RECORD);
    CHECK(g_RecordPanelSlide == 0);

    g_PadPressed = 0;
    g_RecordPanelSlide = -312;
    g_TimeRecordNameCodes[0] = 9;
    UpdateRecordEntry();
    CHECK(g_RecordPanelSlide == -320);
    CHECK(g_RecordEntryState == RECORD_ENTRY_STATE_EDIT_RACE_NAME);
    CHECK(g_NameEntryCursor == 0 && g_NameEntryChar == 9);

    UpdateRecordEntry();
    CHECK(g_RecordEntryState == RECORD_ENTRY_STATE_WAIT_TO_FINISH);
    g_PadPressed = PAD_CONFIRM;
    UpdateRecordEntry();
    CHECK(g_RecordEntryState == RECORD_ENTRY_STATE_FADE_OUT);
    CHECK(s_cdFade == 0x78 && s_cdStarts == 2);

    g_PadPressed = 0;
    g_SceneTimer = 254;
    UpdateRecordEntry();
    CHECK(g_SceneTimer == 256 && g_SceneId == 6);
    CHECK(s_assetRequests == 1);
    CHECK(s_courseIntroDraws == 8);

    puts("record entry scene tests passed");
    return 0;
}
