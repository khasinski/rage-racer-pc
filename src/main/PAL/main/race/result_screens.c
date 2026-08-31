#include <stdio.h>
#include "game/prim.h"
#include "game/audio.h"
#include "game/cd.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/screens.h"
#include "game/state.h"
#include "game/replay_internal.h"
#include "game/save_internal.h"
#include "game/race_internal.h"
#include "game/player_car_internal.h"
#include "game/track.h"

typedef union GrandPrixIntroSelection {
    s16 palette;
    s16 layout;
    s16 width;
    s16 color;
} GrandPrixIntroSelection;

typedef union GrandPrixIntroAddress {
    s16 *value;
    GrandPrixIntroSelection *selection;
} GrandPrixIntroAddress;

void DrawSeriesClearedWash(s32 x, s32 y) {
    void *ot;
    void *prim;
    s32 redStack;
    s32 green;
    s32 temp;
    s32 red;
    s32 blue;
    s32 width;
    s32 height;

    ot = (u8 *)GamePrimaryOrderingTable(0);
    prim = SCRATCH_PRIM_CURSOR_AS(void);

    redStack = y;
    green = x / 8 + redStack;
    blue = x / 4 + redStack;

    if (redStack >= 0) {
        red = redStack;
        if (red >= 0x100) {
            red = 0xFF;
        }
    } else {
        red = 0;
    }
    redStack = red;

    if (green >= 0) {
        temp = green;
        if (temp >= 0x100) {
            temp = 0xFF;
        }
    } else {
        temp = 0;
    }
    green = temp;

    if (blue >= 0) {
        temp = blue;
        if (temp >= 0x100) {
            temp = 0xFF;
        }
    } else {
        temp = 0;
    }

    width = 0x140;
    height = 0xF0;
    prim = GameQueueTileTrans(ot, prim, 0, 0, width, height, redStack, green, temp);
    SCRATCH_PRIM_CURSOR_AS(void) = QueueDrawModePrim(ot, prim, 0x49);
}

void UpdateReplayScene(void) {
    g_AnimTimer++;
    g_SceneTimer++;
    if (g_SceneTimer == 0x3C) {
        if (g_GrandPrixMode != 0) {
            if (g_SeriesCleared == 0) {
                PlaySoundCue(g_RacePosition == 1 ? 0x40 : 0x41);
            }
        }
    }

    if (g_FadeStep < 0) {
        g_FadeLevel += g_FadeStep;
        if (g_FadeLevel < 0) {
            g_FadeLevel = 0;
            g_FadeStep = 0;
            g_EndingWashLevel = 0;
        }
        DrawFullscreenFadeTile(g_FadeLevel, 0x29);
    } else {
        if (g_SeriesCleared != 0) {
            u32 cb = g_ReplayFrameCount;
            u32 fc = g_SceneTimer;
            if (cb - 600 < fc) {
                s32 t = fc + 600;
                s32 c;
                t = t - cb;
                g_EndingWashLevel = t;
                if (t >= 0) {
                    c = t;
                    if (t >= 256) {
                        c = 255;
                    }
                } else {
                    c = 0;
                }
                g_EndingWashLevel = c;
            }
        }

        if (g_FadeStep == 0) {
            if ((g_PadPressed & PAD_CONFIRM) != 0) {
                g_FadeStep = 4;
                StartCdVolumeFade(0x3C);
            } else if (g_SceneTimer == g_ReplayFrameCount - 68) {
                g_FadeStep = 4;
                if (g_ReplayBufferWrapped == 0) {
                    StartCdVolumeFade(0x3C);
                }
            }
        } else {
            g_FadeLevel += g_FadeStep;
            if (g_FadeLevel >= 257) {
                s32 v = g_GrandPrixMode;
                g_MirrorMode = 0;
                g_SceneId = v == 0 ? 0x14 : 0x12;
            }
        }

        if (g_SeriesCleared != 0) {
            u32 replayEnd = g_ReplayFrameCount - 600;
            u32 sceneFrame = g_SceneTimer;
            if ((replayEnd < sceneFrame) || (g_FadeLevel != 0)) {
                DrawSeriesClearedWash(g_EndingWashLevel, g_FadeLevel);
            }
        } else {
            if (g_FadeLevel != 0) {
                DrawFullscreenFadeTile(g_FadeLevel, 0x49);
            }
        }
    }

    {
        ReplayCarAddress player;
        ReplayCarAddress rivals;

        rivals.rivals = g_Cars;
        player.player = &g_PlayerCar;
        ApplyReplayFrame(g_ReplayReadCursor, player.state, rivals.state);
    }
    g_ReplayReadCursor++;
    if (g_ReplayReadCursor == g_ReplayFrameCount) {
        g_ReplayReadCursor = 0;
    }
    UpdateReplayCars();
    UpdateCamera(CAMERA_VIEW_TRACK, (GameRenderObject *)&g_PlayerCar);
    SCRATCH_ENV_MODE4 = g_IsEnvironmentMode4;
    DrawTerrainCellsWide();
    if (g_GrandPrixMode != 0) {
        DrawPlayerCarOnly();
    }
    DrawCourseObjects();
    DrawCourseScenery2(g_SceneTimer, 1);
    UpdateEnvironment();
    DrawSkyBackground();
    DrawReplayBadge();
    if (g_SceneTimer == 1) {
        SetTrackTexturePageNow(g_PlayerCar.trackSection);
    }
}

void DrawResultScreen(void) {
    u8 *base;
    u8 **scratch;
    s32 width;
    s32 y;
    u8 *next;

    DrawProportionalText(0xDC, 0x1C, g_TextResult, 0x7812);

    if (g_GrandPrixMode != 0) {
        y = 0x3C;
    } else {
        y = 0x39;
    }
    DrawText8x8Trans(0x60, y, g_CourseNames[g_CourseIndex], 0x78CC);

    width = 0x140;
    base = (u8 *)GamePrimaryOrderingTable(0);
    scratch = &SCRATCH_PRIM_CURSOR_AS(u8);

    next = *scratch;
    next = AddTilePrim(base, next, 0, 0, width, 0x30, 0x85, 0x15, 0xE);
    *scratch = AddTilePrim(base, next, 0, 0x30, width, 0x18, 0xF0, 0xF0, 0xF0);
}

void DrawGrandprixIntro(void) {
    u8 *base;
    char text[0x30];
    if ((g_ClassResultPlace != 0) &&
        (g_PrizeScreenState >= PRIZE_SCREEN_STATE_WAIT_FOR_BONUS_CONFIRM)) {
        u8 **scratch;
        u8 *next;
        s32 height;
        s32 color;

        scratch = &SCRATCH_PRIM_CURSOR_AS(u8);
        base = (u8 *)GamePrimaryOrderingTable(0);
        height = 8;
        color = 0x78CB;
        next = GameQueueSprite(
            base, *scratch, 0x14, 0x1C, 0x38, height, 0, 0xE8, color);
        next = GameQueueSprite(
            base,
            next,
            0x4C,
            0x1C,
            g_ClassPlaceBarSizes[g_ClassResultPlace - 1].right,
            height,
            0x84,
            g_ClassPlaceBarSizes[g_ClassResultPlace - 1].left,
            color);
        next = GameQueueSprite(
            base,
            next,
            g_ClassPlaceBarSizes[g_ClassResultPlace - 1].right + 0x4E,
            0x1C,
            0x30,
            height,
            0,
            0xF0,
            color);
        next = GameQueueSprite(
            base,
            next,
            g_ClassPlaceBarSizes[g_ClassResultPlace - 1].right + 0x7C,
            0x1C,
            0x20,
            height,
            0,
            0xF8,
            color);
        *scratch = next;
    }

    {
        char *name;
        s32 classNumber;
        s32 current;

        current = g_GrandPrixClass;
        classNumber = current + 1;
        name = g_GrandPrixNames[g_GrandPrixSeries ? current + 6 : current];
        sprintf(text, g_FmtClassGrandPrix, classNumber, name);
    }
    DrawText8x8Trans(0x10, 0x34, text, 0x78CC);

    sprintf(text, g_FmtRoundIn, g_GrandPrixRound);
    DrawText8x8Trans(0x10, 0x3C, text, 0x78CC);

    {
        u8 **scratch;
        GrandPrixIntroSelection *selection;
        GrandPrixIntroAddress selectionAddress;
        u8 *next;
        s32 selectionIndex;

        scratch = &SCRATCH_PRIM_CURSOR_AS(u8);
        DrawResultScreen();

        base = (u8 *)GamePrimaryOrderingTable(0);
        selectionAddress.value = &g_RacePosition;
        selection = selectionAddress.selection;
        next = GameQueueSprite(
            base,
            *scratch,
            0xB4,
            0x60,
            0x58,
            0x38,
            0xA8,
            0xA8,
            g_ResultPanelCluts[selection->palette]);

        selectionIndex = selection->layout;
        selectionIndex -= 1;
        next = GameQueueSprite(
            base,
            next,
            g_ResultPlaceSprites[selectionIndex].x,
            0x5C,
            g_ResultPlaceSprites[selectionIndex].y,
            0x1C,
            g_ResultPlaceSprites[selection->width - 1].width,
            0xCC,
            g_ResultPlaceCluts[selection->color]);
        *scratch = next;
    }

    DrawProportionalText(0x10, 0x50, g_CaptionRanking, 0x7812);
}

void DrawRaceTimePanel(s32 slideY) {
    s32 base;
    s32 i;
    s32 *times;
    s32 *selectedPtr;
    s32 count;
    s32 x;
    s32 textPos;
    s32 column;
    s32 columnBase;
    s32 drawColor;
    s32 quotient;
    PlayerLapTimeAddress highlightAddress;
    char text[24];
    s32 color;

    base = slideY;
    DrawProportionalText(0x10, base + 0x80, g_CaptionTotalTime, 0x7812);

    text[0] = 0x54;
    text[1] = 0x2F;
    FormatLapTime(&text[2], g_RaceTotalTime);

    color = 0x7812;
    if (g_BestTotalTimes[g_GrandPrixSeries][SeriesCourseIndex()][g_GrandPrixMode] == g_RaceTotalTime) {
        color = 0x784C;
    }
    drawColor = color;
    count = base + 0x90;
    DrawProportionalText(0x14, count, text, drawColor);

    DrawProportionalText(0x10, base + 0xA4, g_CaptionLapTime, 0x7812);

    count = 6;
    if (g_CourseIndex != 3) {
        count = 3;
    }

    i = 0;
    if (count != 0) {
        times = g_PlayerCar.lapTimes.table.milliseconds;
        selectedPtr = g_PlayerCar.lapTimes.table.milliseconds;
        do {
            x = 0xB0;
            if (i < 3) {
                x = 0x14;
            }
            quotient = i / 3;
            column = i - quotient * 3;
            text[0] = i + 0x31;
            columnBase = 0xB0;
            textPos = column * 0xC + (base + columnBase);
            FormatLapTime(&text[2], *times);
            color = 0x7812;
            highlightAddress.timePointer = selectedPtr;
            highlightAddress.bytes -= PLAYER_LAP_HIGHLIGHT_TO_MILLISECONDS;
            if (*highlightAddress.halfwordPointer == i) {
                color = 0x784C;
            }
            DrawProportionalText(x, textPos, text, color);
            i++;
            times++;
        } while (i < count);
    }
}
