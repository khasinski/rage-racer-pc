#ifndef GAME_RACE_HUD_INTERNAL_H
#define GAME_RACE_HUD_INTERNAL_H

#include "common.h"

u8 *DrawHudDigit(u8 *packet, s32 x, s32 y, s32 digit, u16 clut);
void DrawSpeedDigits(s32 x, s32 y, s32 speed);
void DrawSplitTimes(void);
void DrawSplitIndicator(s32 sectorIndex, s32 direction);
void DrawStartCountdown(s32 sceneTimer);
void BuildTileStrips(void);
/* Draws the in-race option overlay. The "RAGE RACER GE" text is one half of
 * its scrolling marquee, not a title-screen label. */
void DrawRaceOptionMenu(s32 cursorRow);
s32 SplitCurrentTimeVisible(s32 timer, s32 sectorIndex);
s32 SplitDeltaVisible(s32 timer, s32 sectorIndex, s32 sign,
                      s32 lapCount, s32 playerLap);
s32 SplitDeltaClut(s32 sign);
s32 SplitTimeClut(s32 timeMs);
s32 SplitRecordSeriesIndex(s32 series);
s32 SplitDisplaySectorIndex(s32 sector);

typedef struct StartCountdownTiming {
    s32 visible;
    s32 phase;
    s32 wipeHalfStep;
} StartCountdownTiming;

StartCountdownTiming CalculateStartCountdownTiming(s32 sceneTimer);
s32 CountdownTileBufferIndex(s32 frameParity);

typedef struct StartCountdownRow {
    u32 pattern;
    s32 colorBank;
} StartCountdownRow;

StartCountdownRow BuildStartCountdownRow(s32 phase, s32 row,
                                         s32 wipeHalfStep,
                                         const u32 *glyphPatterns,
                                         const u32 *firstPattern);
s32 AdvanceStartCountdownBoard(s32 phase, s32 currentOffset);

typedef struct StartCountdownLamp {
    s32 intensity;
    u16 clut;
} StartCountdownLamp;

StartCountdownLamp BuildStartCountdownLamp(s32 phase, s32 sceneTimer,
                                           s32 lampIndex);

typedef struct RaceOptionMarqueeState {
    s32 firstScroll;
    s32 secondScroll;
    s32 brightness;
    s32 textFrame;
} RaceOptionMarqueeState;

RaceOptionMarqueeState AdvanceRaceOptionMarquee(s32 firstScroll,
                                                s32 secondScroll,
                                                s32 sceneTimer);

typedef struct RaceOptionPulseState {
    s32 angle;
    s32 halfWidth;
} RaceOptionPulseState;

RaceOptionPulseState AdvanceRaceOptionPulse(s32 angle);

#endif
