#include "common.h"
#include "game/race.h"
#include "game/player_car_internal.h"
#include "game/track.h"
#include "game/audio.h"

void UpdateZoneAmbience(s32 zone) {
    s32 position;
    s32 base;
    register s32 mode asm("$4");
    s32 value;
    s32 finalValue;
    s32 i;
    s32 sentinel;
    TrackAmbienceZone *entryBase;
    register TrackAmbienceZone *entry asm("$3");
    s32 selector;

    position = zone;
    selector = g_GrandPrixClass;
    entryBase = g_TrackEventData->ambienceZones;
    selector = selector % 5;

    switch (selector) {
    default:
    case 0:
    case 5:
        base = 0;
        mode = 0;
        break;
    case 1:
    case 2:
        base = 0x60;
        mode = 0;
        break;
    case 3:
    case 4:
        base = 0x60;
        mode = 1;
        break;
    }

    if (g_RaceSeries != 0) {
        position = g_TrackLength - position;
    }

    value = 0;
    i = 0;
    sentinel = -1;
    entry = entryBase;
    while (i < 4) {
        s32 start;
        s32 end;

        start = entry->start;
        end = entry->end;
        if (start == sentinel) {
            break;
        }
        if (!(position < start) && !(end < position)) {
            s32 flags;
            s32 delta;
            s32 product;
            s32 flagBit;

            flags = entry->flags;
            if (position < start + 0x320 && (flags & 1) > 0) {
                delta = position - start;
                product = base * delta;
                value = product / 800;
            } else if (end - 0x320 < position && ((flagBit = flags & 2), flagBit > 0)) {
                delta = end - position;
                product = base * delta;
                value = product / 800;
            } else {
                value = base;
            }
            break;
        }
        i++;
        entry++;
    }

    finalValue = value;
    finalValue = (s16)finalValue;
    SetStereoSoundCue(mode, finalValue, finalValue);
}


void TriggerRaceCues(void) {
    TrackRaceCueAddress base;
    s32 i;
    s32 mask;
    PlayerRaceCueState *state;
    register s32 temp asm("v0");
    register TrackRaceCueAddress entry asm("v1");
    register s32 loopFlags asm("t0");
    register s32 current asm("a0");
    s32 product;
    TrackRaceCueAddress finishAddress;

    current = g_RaceCueFlags;
    base.pointer = &g_TrackEventData->raceCues;

    if (!(current & 8)) {
        finishAddress.finishPointer = &base.pointer->finish[g_RaceSeries];
        if (g_PlayerCar.trackSection == finishAddress.finishPointer->trackSection) {
            entry.value = g_PlayerCar.lap;
            if (entry.value == g_LapCount) {
                entry.value = current | 8;
                g_RaceCueFlags = entry.value;
                if (g_WrongWayTimer < 10) {
                    PlaySoundCue(0x2A);
                }
            }
        }
    }

    if (g_WrongWayTimer != 0) {
        return;
    }

    state = GetPlayerRaceCueState(&g_PlayerCar);
    i = 0;
    temp = 0x10;
    do {
        loopFlags = g_RaceCueFlags;
        mask = temp << i;
        temp = mask & loopFlags;
        if (temp == 0) {
            temp = g_RaceSeries;
            entry.bytePointer =
                (u8 *)&base.pointer->speed[g_RaceSeries][i];
            current = ((TrackSpeedCue *)entry.bytePointer)->trackSection;
            temp = -1;
            if (current == temp) {
                return;
            }

            temp = state->trackSection;
            if (temp == current) {
                entry.value = ((TrackSpeedCue *)entry.bytePointer)->speedPercent;
                temp = state->speedScale;
                product = entry.value * temp;
                temp = product / 100;
                entry.value = state->speed;
                if (temp < entry.value) {
                    temp = state->motionMode;
                    if (temp <= 0) {
                        temp = mask | loopFlags;
                        g_RaceCueFlags = temp;
                        PlaySoundCue(0x23);
                    }
                }
                return;
            }
        }
        i++;
        temp = i < 3;
        if (temp == 0) {
            break;
        }
        temp = 0x10;
    } while (1);
}
