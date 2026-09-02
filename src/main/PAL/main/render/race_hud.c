#include "game/prim.h"
#include "game/race.h"
#include "game/player_car_internal.h"
#include "game/race_hud_internal.h"
#include "game/render.h"
#include "game/render_internal.h"

#include "rage/hud_config.h"

enum {
    HUD_LAP_TIME_DESC_COUNT = 6,
    TIME_ATTACK_STATIC_LABEL_COUNT = 3,
    GRAND_PRIX_STATIC_LABEL_COUNT = 6,
    GRAND_PRIX_LAP_TIMES_LABEL = 1,
    GRAND_PRIX_TIME_LIMIT_LABEL = 2,
};

static s32 RaceHudLabelVisible(s32 grandPrixMode, s32 label) {
    if (grandPrixMode == 0) {
        return HudShowLapTimes();
    }
    if (label == GRAND_PRIX_LAP_TIMES_LABEL) {
        return HudShowLapTimes();
    }
    if (label == GRAND_PRIX_TIME_LIMIT_LABEL) {
        return HudShowTimeLimit();
    }
    return 1;
}

void DrawRaceHudLabels(s32 grandPrixMode) {
    s32 labelCount = grandPrixMode != 0 ? GRAND_PRIX_STATIC_LABEL_COUNT
                                        : TIME_ATTACK_STATIC_LABEL_COUNT;
    const GameSpriteDesc *descs = grandPrixMode != 0
                                      ? g_RaceHudSpriteDescsGp
                                      : g_RaceHudSpriteDescsTimeTrial;
    GameFrameContext *frame = g_DrawBuffer;
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);
    s32 label;

    for (label = 0; label < labelCount; label++) {
        SPRT *sprite = &frame->layout.raceHud.labels[label];

        sprite->x0 = HudAnchorX(descs[label + HUD_LAP_TIME_DESC_COUNT].x);
        if (RaceHudLabelVisible(grandPrixMode, label)) {
            AddPrim(ot, sprite);
        }
    }

    g_RenderState.packetCursor = QueueDrawModePrim(
        ot, RENDER_PRIM_CURSOR_AS(u8), 9);
}


/* The lap-time column: one row per lap from the player timing table at x=0xFA,
 * y stepping 0xA, the current lap highlighted and unset laps drawn as -1. */
void DrawLapTimes(void) {
    s32 visibleCount = g_PlayerCar.lap;
    s32 activeLap = g_PlayerCar.drive.hudLapHighlightRow;
    GameFrameContext *frame = g_DrawBuffer;
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);
    const GameSpriteDesc *descs = g_GrandPrixMode != 0
                                      ? g_RaceHudSpriteDescsGp
                                      : g_RaceHudSpriteDescsTimeTrial;
    s32 lap;

    if (!HudShowLapTimes()) {
        return;
    }

    if (visibleCount > g_LapCount) {
        visibleCount = g_LapCount;
    }

    for (lap = 0; lap < g_LapCount; lap++) {
        s32 lapTime = g_PlayerCar.lapTimes.table.milliseconds[lap];
        s32 color = lap == activeLap ? 0x780F
                    : lapTime >= RACE_TIME_MAX_MS ? 0x7890
                                                  : 0x78CC;
        SPRT *sprite = &frame->layout.raceHud.lapTimes[lap];

        DrawTimeValue(HudRightX(0xFA), 0x2E + lap * 0xA,
                      lap < visibleCount ? lapTime : -1, color, 0x3E8);
        sprite->x0 = HudAnchorX(descs[lap].x);
        sprite->clut = color;
        AddPrim(ot, sprite);
    }

    DrawTimeValue(HudRightX(0xFA), 0x20, g_BestLapThisRace,
                  0x78CC, 0x3E8);
}

void DrawTimeRemaining(s32 time) {
    s32 clutIndex = 0x78CC;

    if (!HudShowTimeLimit()) return;

    if (time < 0x5DC) {
        clutIndex = 0x7811;
    }

    DrawMinuteSecondTime(HudLeftX(0xE), 0xD2, time, clutIndex);
}

/* The two race-position digits, from g_PlayerCar.drive.racePosition; the tens digit is
 * blanked below 10 and the colour changes from 4th place down. */
void DrawRacePosition(void) {
    GameFrameContext *frame = g_DrawBuffer;
    SPRT *tens = &frame->layout.raceHud.labels[3];
    SPRT *ones = &frame->layout.raceHud.labels[4];
    u16 color = g_PlayerCar.drive.racePosition < 4 ? 0x780B : 0x780E;

    tens->u0 = g_PlayerCar.drive.racePosition >= 10 ? 0x18 : 0;
    ones->u0 = (g_PlayerCar.drive.racePosition % 10) * 24;
    tens->clut = color;
    ones->clut = color;
}

void DrawSplitIndicator(s32 sector, s32 sign) {
    GameFrameContext *frame = g_DrawBuffer;
    SPRT *sectorDigit = &frame->layout.raceHud.labels[3];
    SPRT *signSprite = &frame->layout.raceHud.labels[4];
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);

    sectorDigit->u0 = sector * 8 + 0x50;
    AddPrim(ot, sectorDigit);

    if (sign > 0) {
        signSprite->u0 = 0x88;
        signSprite->clut = 0x7810;
    } else if (sign < 0) {
        signSprite->u0 = 0x78;
        signSprite->clut = 0x780F;
    } else {
        return;
    }

    AddPrim(ot, signSprite);
}
