#include <stdio.h>
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

typedef union RankingTextBuffer {
    char value[56];
    char first;
} RankingTextBuffer;

void DrawRankingPanel(s32 slideX) {
    s32 panel;
    s32 iter;
    s32 countOrIndex;
    s32 xOrField;
    s32 destination;
    s32 color;
    s32 *lapTime;
    s32 carNameY;
    RankingTextBuffer text;
    s32 mode;
    s32 row;
    s32 doubledRow;
    s32 value;
    s32 limit;

    panel = slideX;
    DrawProportionalText(panel + 0x10, 0x4C, g_CaptionLapTime2, 0x7852);
    mode = g_CourseIndex;
    text.value[1] = 0x2F;
    limit = 6;
    if (mode != 3) {
        limit = 3;
    }
    iter = 0;
    if (limit > 0) {
        lapTime = g_PlayerCar.lapTimes.table.milliseconds;
        do {
            row = iter / 2;
            doubledRow = row * 2;
            value = iter - doubledRow;
            value <<= 3;
            xOrField = value + 0x58;
            text.first = iter + 0x31;
            doubledRow = (doubledRow + row) << 5;
            value = panel + 0x14;
            FormatLapTime(&text.value[2], *lapTime);
            destination = doubledRow;
            destination = destination + value;
            color = 0x78CC;
            if (g_BestLapIndex == iter) {
                color = 0x780F;
            }
            DrawText8x8(destination, xOrField, text.value, color);
            iter++;
            lapTime++;
        } while (iter < limit);
    }
    DrawProportionalText(panel + 0x10, 0x6C, g_CaptionRanking2, 0x7812);
    countOrIndex = 0;
    carNameY = 0x82;
    destination = 0x78;
    do {
        text.value[0] = g_PlaceSuffixNames[countOrIndex][0];
        text.value[1] = g_PlaceSuffixNames[countOrIndex][1];
        text.value[2] = g_PlaceSuffixNames[countOrIndex][2];
        text.value[3] = 0x2F;
        FormatLapTime(&text.value[4], g_RankingRecords[g_GrandPrixSeries][SeriesCourseIndex()][countOrIndex].raceTime);
        xOrField = g_RankingRecords[g_GrandPrixSeries][SeriesCourseIndex()][countOrIndex].carIndex;
        sprintf(&text.value[0xC], g_FmtRecordName,
                      &g_RankingRecords[g_GrandPrixSeries][SeriesCourseIndex()][countOrIndex],
                      g_CarClassNames[xOrField]);
        color = 0x78CC;
        if (g_RankingInsertRow == countOrIndex) {
            color = 0x780F;
        }
        DrawText8x8(panel + 0x14, destination, text.value, color);
        sprintf(text.value, g_FmtCarName, g_CarNames[xOrField]);
        DrawText8x8(panel + 0x2C, carNameY, text.value, color);
        destination += 0x14;
        carNameY += 0x14;
        countOrIndex++;
    } while (countOrIndex < 5);
}

void DrawTimeRecordPanel(s32 s5) {
    char text[48];
    s32 s4, s3;
    s32 s2, color, idx;

    DrawProportionalText(s5 + 0x10, 0x4C, g_CaptionTotalTime2, 0x7852);

    text[0] = 0x54;
    text[1] = 0x2F;
    FormatLapTime(&text[2], g_RaceTotalTime);
    DrawText8x8(s5 + 0x14, 0x58, text, 0x78CC);

    DrawProportionalText(s5 + 0x10, 0x6C, g_CaptionRanking2, 0x7812);

    s2 = 0;
    s4 = 0x82;
    s3 = 0x78;
    for (; s2 < 5; s2++) {
        text[0] = g_PlaceSuffixNames[s2][0];
        text[1] = g_PlaceSuffixNames[s2][1];
        text[2] = g_PlaceSuffixNames[s2][2];
        text[3] = 0x2F;
        FormatLapTime(&text[4], g_TimeRecords[g_GrandPrixSeries][SeriesCourseIndex()][s2].raceTime);

        idx = g_TimeRecords[g_GrandPrixSeries][SeriesCourseIndex()][s2].carIndex;
        sprintf(&text[0xC], g_FmtRecordName,
                      &g_TimeRecords[g_GrandPrixSeries][SeriesCourseIndex()][s2], g_CarClassNames[idx]);

        color = 0x78CC;
        if (g_TimeRecordInsertRow == s2) {
            color = 0x780F;
        }
        DrawText8x8(s5 + 0x14, s3, text, color);

        sprintf(text, g_FmtCarName, g_CarNames[idx]);

        DrawText8x8(s5 + 0x2C, s4, text, color);
        s3 += 0x14;
        s4 += 0x14;
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
    s32 count;
    s32 i;
    s32 best;
    s32 *score_ptr;
    s32 course;

    count = 3;
    if (g_CourseIndex == 3) {
        count = 6;
    }

    best = 0x927C0;
    i = 0;
    if (i < count) {
        score_ptr = g_PlayerCar.lapTimes.table.milliseconds;
        while (i < count) {
            if (*score_ptr < best) {
                best = *score_ptr;
                g_BestLapIndex = i;
            }
            i++;
            score_ptr++;
        }
    }

    course = SeriesCourseIndex();
    g_RankingInsertRow = InsertRaceRecord(
        g_RankingRecords[g_GrandPrixSeries][course], best, g_PlayerCarIndex,
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
    u8 *name;
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
        u8 *timeName;
        s32 previous;
        u16 buttons;

        previous = g_NameEntryChar;
        if (g_PadPressedRepeat & PAD_LEFT) {
            g_NameEntryChar = previous - 1;
        } else if (g_PadPressedRepeat & PAD_RIGHT) {
            g_NameEntryChar = previous + 1;
        }
        g_NameEntryChar = (g_NameEntryChar + 42) % 42;
        if (previous != g_NameEntryChar) {
            PlaySoundCue(1);
        }

        g_RankingNameCodes[g_NameEntryCursor] = g_NameEntryChar;
        buttons = g_PadPressed;
        name = g_RankingNameCodes;
        if (buttons & 0x860) {
            PlaySoundCue(2);
            g_NameEntryCursor++;
            if (g_NameEntryCursor == 6) {
                g_RecordEntryState = RECORD_ENTRY_STATE_WAIT_AFTER_LAP_NAME;
                i = 0;
                if (g_TimeRecordInsertRow < 5) {
                    timeName = g_TimeRecordNameCodes;
                    do {
                        *timeName = g_RankingNameCodes[i];
                        g_TimeRecords[g_GrandPrixSeries][SeriesCourseIndex()]
                                     [g_TimeRecordInsertRow].driverName[i] =
                            g_NameEntryCharset[*timeName];
                        i++;
                        timeName++;
                    } while (i < 6);
                }
            }
            g_NameEntryChar = g_RankingNameCodes[g_NameEntryCursor];
        } else if ((buttons & 0x90) && g_NameEntryCursor > 0) {
            PlaySoundCue(3);
            g_NameEntryCursor--;
            g_NameEntryChar = name[g_NameEntryCursor];
        }

        if (g_RecordEntryState == RECORD_ENTRY_STATE_EDIT_LAP_NAME) {
            DrawNameEntryCursor(g_NameEntryCursor, g_RankingInsertRow);
        }
        for (i = 0; i < 6; i++) {
            g_RankingRecords[g_GrandPrixSeries][SeriesCourseIndex()]
                            [g_RankingInsertRow].driverName[i] =
                g_NameEntryCharset[g_RankingNameCodes[i]];
        }
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
        s32 previous;
        u16 buttons;

        previous = g_NameEntryChar;
        if (g_PadPressedRepeat & PAD_LEFT) {
            g_NameEntryChar = previous - 1;
        } else if (g_PadPressedRepeat & PAD_RIGHT) {
            g_NameEntryChar = previous + 1;
        }
        g_NameEntryChar = (g_NameEntryChar + 42) % 42;
        if (previous != g_NameEntryChar) {
            PlaySoundCue(1);
        }

        g_TimeRecordNameCodes[g_NameEntryCursor] = g_NameEntryChar;
        buttons = g_PadPressed;
        name = g_TimeRecordNameCodes;
        if (buttons & 0x860) {
            PlaySoundCue(2);
            g_NameEntryCursor++;
            if (g_NameEntryCursor == 6) {
                g_RecordEntryState = RECORD_ENTRY_STATE_WAIT_TO_FINISH;
            }
            g_NameEntryChar = name[g_NameEntryCursor];
        } else if ((buttons & 0x90) && g_NameEntryCursor > 0) {
            PlaySoundCue(3);
            g_NameEntryCursor--;
            g_NameEntryChar = name[g_NameEntryCursor];
        }

        if (g_RecordEntryState == RECORD_ENTRY_STATE_EDIT_RACE_NAME) {
            DrawNameEntryCursor(g_NameEntryCursor, g_TimeRecordInsertRow);
        }
        for (i = 0; i < 6; i++) {
            g_TimeRecords[g_GrandPrixSeries][SeriesCourseIndex()]
                         [g_TimeRecordInsertRow].driverName[i] =
                g_NameEntryCharset[g_TimeRecordNameCodes[i]];
        }
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
        {
            u32 sceneFrame = g_SceneTimer;
            if (sceneFrame >= 0x100) {
                RequestSelectBgmAssets();
                g_SceneId = 6;
            }
        }
        DrawTimeRecordPanel(0);
        break;
    }

    DrawCourseIntro();
}

void ReturnFromClassFmv(void) {
    CdSync(0, 0);
    CdControl(9, 0, 0);
    g_SceneId = 6;
    RequestSelectBgmAssets();
}

void ReturnFromEndingFmv(void) {
    CdSync(0, 0);
    CdControl(9, 0, 0);
    SetDispMask(0);
    SetupDisplay240(0, 0, 0);
    g_FrameSyncThreshold = 0x80;
    g_FadeStep = 4;
    g_FadeLevel = 0;
    g_SceneId = 0x22;
    g_SceneTimer = 0;
}
