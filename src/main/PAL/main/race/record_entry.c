#include "common.h"
#include "game/game_input.h"
#include <stdio.h>
#include "game/prim.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/player_car_internal.h"
#include "game/cd.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/records_internal.h"
#include "game/race_internal.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/render_workspace.h"
#include "game/screens.h"
#include "game/state.h"
#include "game/game_context.h"
#include "psyq/cd.h"
#include "psyq/gpu.h"

typedef union SignedDivisionWork {
    s32 value;
    u32 unsignedValue;
} SignedDivisionWork;

typedef union RankingTextBuffer {
    char value[56];
    volatile char first;
} RankingTextBuffer;

void DrawRankingPanel(s32 slideX) {
    s32 panel;
    SignedDivisionWork iter;
    s32 countOrIndex;
    s32 xOrField;
    s32 destination;
    s32 color;
    PlayerLapTimeAddress scoreOrX;
    RankingTextBuffer text;
    s32 mode;
    s32 row;
    s32 doubledRow;
    s32 value;
    s32 scoreValue;
    s32 limit;

    panel = slideX;
    DrawProportionalText(panel + 0x10, 0x4C, g_CaptionLapTime2, 0x7852);
    mode = g_CourseIndex;
    text.value[1] = 0x2F;
    limit = 6;
    if (mode != 3) {
        limit = 3;
    }
    iter.value = 0;
    if (limit > 0) {
        scoreOrX.timePointer = g_PlayerCar.lapTimes.table.milliseconds;
        do {
            /* row = iter / 2 and value = (iter % 2) * 8: two lap times per
             * table row, the odd one 8 px to the right. This is gcc's own
             * expansion of the signed divide; writing it back as `iter / 2`
             * does not re-expand to the same code here, so it stays. */
            scoreValue = iter.value + (iter.unsignedValue >> 31);
            row = scoreValue >> 1;
            doubledRow = row * 2;
            value = iter.value - doubledRow;
            value <<= 3;
            xOrField = value + 0x58;
            text.first = iter.value + 0x31;
            doubledRow = (doubledRow + row) << 5;
            scoreValue = *scoreOrX.timePointer;
            value = (destination = panel + 0x14);
            FormatLapTime(&text.value[2], scoreValue);
            destination = doubledRow;
            destination = destination + value;
            color = 0x78CC;
            if (g_BestLapIndex == iter.value) {
                color = 0x780F;
            }
            DrawText8x8(destination, xOrField, text.value, color);
            iter.value++;
            scoreOrX.timePointer++;
        } while (iter.value < limit);
    }
    DrawProportionalText(panel + 0x10, 0x6C, g_CaptionRanking2, 0x7812);
    countOrIndex = 0;
    scoreOrX.value = 0x82;
    destination = 0x78;
    do {
        text.value[0] = g_PlaceSuffixNames[countOrIndex][0];
        text.value[1] = g_PlaceSuffixNames[countOrIndex][1];
        text.value[2] = g_PlaceSuffixNames[countOrIndex][2];
        text.value[3] = 0x2F;
        FormatLapTime(&text.value[4], g_RankingRecords[g_GrandPrixSeries][RageSeriesCourseIndex()][countOrIndex].raceTime);
        xOrField = g_RankingRecords[g_GrandPrixSeries][RageSeriesCourseIndex()][countOrIndex].carIndex;
        sprintf(&text.value[0xC], g_FmtRecordName,
                      &g_RankingRecords[g_GrandPrixSeries][RageSeriesCourseIndex()][countOrIndex],
                      g_CarClassNames[xOrField]);
        color = 0x78CC;
        if (g_RankingInsertRow == countOrIndex) {
            color = 0x780F;
        }
        DrawText8x8(panel + 0x14, destination, text.value, color);
        sprintf(text.value, g_FmtCarName, g_CarNames[xOrField]);
        DrawText8x8(panel + 0x2C, scoreOrX.value, text.value, color);
        destination += 0x14;
        scoreOrX.value += 0x14;
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
        FormatLapTime(&text[4], g_TimeRecords[g_GrandPrixSeries][RageSeriesCourseIndex()][s2].raceTime);

        idx = g_TimeRecords[g_GrandPrixSeries][RageSeriesCourseIndex()][s2].carIndex;
        sprintf(&text[0xC], g_FmtRecordName,
                      &g_TimeRecords[g_GrandPrixSeries][RageSeriesCourseIndex()][s2], g_CarClassNames[idx]);

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
    u8 **scratch;

    if (g_AnimTimer & 8) {
        scratch = &RENDER_PRIM_CURSOR_AS(u8);
        *scratch = AddTilePrim(
            GamePrimaryOrderingTable(0),
            *scratch,
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
    s32 row_offset;
    s32 best;
    s32 j;
    s32 *score_ptr;
    register RaceRecordAddress entryAddress asm("$5");
    register s32 score_offset asm("$3");
    s32 score_value;
    s32 mode;
    RaceRecordAddress baseAddress;
    s32 letter;
    s32 code;
    s32 letter2;
    s32 code2;
    RaceRecordAddress recordAddress;
    RaceRecordAddress rankingBaseAddress;
    RaceRecordAddress timeBaseAddress;
    s32 fill_offset;

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

    i = 0;
    rankingBaseAddress.pointer = &g_RankingRecords[0][0][0];
    letter = 0x41;
    code = 0xB;
    row_offset = 0;
    while (i < 5) {
        score_offset = row_offset + (g_CourseIndex * 0x50);
        score_offset += g_GrandPrixSeries * 0x140;
        recordAddress.pointer = &g_RankingRecords[0][0][0];
        recordAddress.bytePointer += score_offset;
        if (best < recordAddress.pointer->raceTime) {
            if (i < 4) {
                j = 4;
                do {
                    entryAddress.recordOffset = j * sizeof(RaceRecord);
                    j--;
                    mode = g_CourseIndex;
                    score_offset =
                        (g_GrandPrixSeries * 0x140) + rankingBaseAddress.value;
                    baseAddress.value = (mode * 0x50) + score_offset;
                    entryAddress.value = entryAddress.recordOffset + baseAddress.value;
                    entryAddress.pointer--;
                    entryAddress.pointer[1] = entryAddress.pointer[0];
                } while (i < j);
            }
            score_offset = row_offset + (g_CourseIndex * 0x50);
            score_offset += g_GrandPrixSeries * 0x140;
            {
                RaceRecordAddress rankingTimeAddress;

                rankingTimeAddress.pointer = &g_RankingRecords[0][0][0];
                rankingTimeAddress.bytePointer += score_offset;
                rankingTimeAddress.pointer->raceTime = best;
            }
            j = 0;
            fill_offset = row_offset;
            for (; j < 6; j++) {
                RaceRecordAddress nameAddress;

                score_offset = fill_offset + (g_CourseIndex * 0x50);
                score_offset += g_GrandPrixSeries * 0x140;
                nameAddress.bytePointer = rankingBaseAddress.bytePointer;
                nameAddress.value = score_offset + nameAddress.value;
                nameAddress.bytePointer += j;
                *nameAddress.volatileBytePointer = letter;
                g_RankingNameCodes[j] = code;
            }

            score_offset = row_offset + (g_CourseIndex * 0x50);
            score_offset += g_GrandPrixSeries * 0x140;
            {
                RaceRecordAddress rankingCarAddress;

                rankingCarAddress.pointer = &g_RankingRecords[0][0][0];
                rankingCarAddress.bytePointer += score_offset;
                rankingCarAddress.pointer->carIndex = g_PlayerCarIndex;
            }
            break;
        }
        i++;
        row_offset += 0x10;
    }

    g_RankingInsertRow = i;
    i = 0;
    timeBaseAddress.pointer = &g_TimeRecords[0][0][0];
    letter2 = 0x41;
    code2 = 0xB;
    row_offset = 0;
    while (i < 5) {
        score_offset = row_offset + (g_CourseIndex * 0x50);
        score_offset += g_GrandPrixSeries * 0x140;
        {
            RaceRecordAddress timeValueAddress;

            timeValueAddress.pointer = &g_TimeRecords[0][0][0];
            timeValueAddress.bytePointer += score_offset;
            score_value = timeValueAddress.pointer->raceTime;
        }
        if (g_RaceTotalTime < score_value) {
            if (i < 4) {
                j = 4;
                do {
                    entryAddress.recordOffset = j * sizeof(RaceRecord);
                    j--;
                    mode = g_CourseIndex;
                    score_offset =
                        (g_GrandPrixSeries * 0x140) + timeBaseAddress.value;
                    baseAddress.value = (mode * 0x50) + score_offset;
                    entryAddress.value = entryAddress.recordOffset + baseAddress.value;
                    entryAddress.pointer--;
                    entryAddress.pointer[1] = entryAddress.pointer[0];
                } while (i < j);
            }
            score_offset = row_offset + (g_CourseIndex * 0x50);
            score_offset += g_GrandPrixSeries * 0x140;
            {
                RaceRecordAddress timeRecordAddress;

                timeRecordAddress.pointer = &g_TimeRecords[0][0][0];
                timeRecordAddress.bytePointer += score_offset;
                timeRecordAddress.pointer->raceTime = g_RaceTotalTime;
            }
            j = 0;
            fill_offset = row_offset;
            for (; j < 6; j++) {
                RaceRecordAddress nameAddress;

                score_offset = fill_offset + (g_CourseIndex * 0x50);
                score_offset += g_GrandPrixSeries * 0x140;
                nameAddress.bytePointer = timeBaseAddress.bytePointer;
                nameAddress.value = score_offset + nameAddress.value;
                nameAddress.bytePointer += j;
                *nameAddress.volatileBytePointer = letter2;
                g_TimeRecordNameCodes[j] = code2;
            }

            score_offset = row_offset + (g_CourseIndex * 0x50);
            score_offset += g_GrandPrixSeries * 0x140;
            {
                RaceRecordAddress timeCarAddress;

                timeCarAddress.pointer = &g_TimeRecords[0][0][0];
                timeCarAddress.bytePointer += score_offset;
                timeCarAddress.pointer->carIndex = g_PlayerCarIndex;
            }
            break;
        }
        i++;
        row_offset += 0x10;
    }

    g_TimeRecordInsertRow = i;
}

void EnterRecordEntry(void) {
    g_SceneTimer = 0x100;
    g_FrameSyncThreshold = 0x80;
    g_RecordEntryState = RECORD_ENTRY_STATE_FADE_IN;
    GameSceneSet(SCENE_RECORD);
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
        RaceRecordAddress timeRecordBase;
        RaceRecordAddress recordAddress;
        s32 previous;
        u16 buttons;

        previous = g_NameEntryChar;
        if (g_GameInput.pressedRepeat & PAD_LEFT) {
            g_NameEntryChar = previous - 1;
        } else if (g_GameInput.pressedRepeat & PAD_RIGHT) {
            g_NameEntryChar = previous + 1;
        }
        g_NameEntryChar = (g_NameEntryChar + 42) % 42;
        if (previous != g_NameEntryChar) {
            PlaySoundCue(1);
        }

        g_RankingNameCodes[g_NameEntryCursor] = g_NameEntryChar;
        buttons = g_GameInput.pressed;
        name = g_RankingNameCodes;
        if (buttons & 0x860) {
            PlaySoundCue(2);
            g_NameEntryCursor++;
            if (g_NameEntryCursor == 6) {
                g_RecordEntryState = RECORD_ENTRY_STATE_WAIT_AFTER_LAP_NAME;
                i = 0;
                if (g_TimeRecordInsertRow < 5) {
                    timeRecordBase.pointer = &g_TimeRecords[0][0][0];
                    timeName = g_TimeRecordNameCodes;
                    do {
                        *timeName = g_RankingNameCodes[i];
                        recordAddress.value =
                            (((g_CourseIndex * 5) + g_TimeRecordInsertRow) *
                            sizeof(RaceRecord)) +
                            (g_GrandPrixSeries * sizeof(g_TimeRecords[0])) +
                            timeRecordBase.value;
                        recordAddress.bytePointer += i;
                        i++;
                        *recordAddress.bytePointer = g_NameEntryCharset[*timeName];
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
        i = 0;
        do {
            g_RankingRecords[g_GrandPrixSeries][RageSeriesCourseIndex()]
                            [g_RankingInsertRow].driverName[i] =
                g_NameEntryCharset[g_RankingNameCodes[i]];
            i++;
        } while (i < 6);
        DrawRankingPanel(0);
        break;
    }

    case RECORD_ENTRY_STATE_WAIT_AFTER_LAP_NAME:
        if (g_GameInput.pressed & PAD_CONFIRM) {
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
        if (g_GameInput.pressedRepeat & PAD_LEFT) {
            g_NameEntryChar = previous - 1;
        } else if (g_GameInput.pressedRepeat & PAD_RIGHT) {
            g_NameEntryChar = previous + 1;
        }
        g_NameEntryChar = (g_NameEntryChar + 42) % 42;
        if (previous != g_NameEntryChar) {
            PlaySoundCue(1);
        }

        g_TimeRecordNameCodes[g_NameEntryCursor] = g_NameEntryChar;
        buttons = g_GameInput.pressed;
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
        i = 0;
        do {
            g_TimeRecords[g_GrandPrixSeries][RageSeriesCourseIndex()]
                         [g_TimeRecordInsertRow].driverName[i] =
                g_NameEntryCharset[g_TimeRecordNameCodes[i]];
            i++;
        } while (i < 6);
        DrawTimeRecordPanel(0);
        break;
    }

    case RECORD_ENTRY_STATE_WAIT_TO_FINISH:
        if (g_GameInput.pressed & PAD_CONFIRM) {
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
                GameSceneSet(SCENE_MENU_ENTER);
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
    GameSceneSet(SCENE_MENU_ENTER);
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
    GameSceneSet(SCENE_ENDING_STILL);
    g_SceneTimer = 0;
}
