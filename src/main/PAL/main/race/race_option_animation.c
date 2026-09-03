#include "game/race_hud_internal.h"
#include "psyq/gte.h"

static s32 AdvanceMarqueeLine(s32 scroll) {
    if (scroll <= -624 || scroll > 240) {
        return 240;
    }
    return scroll - 4;
}

RaceOptionMarqueeState AdvanceRaceOptionMarquee(s32 firstScroll,
                                                s32 secondScroll,
                                                s32 sceneTimer) {
    RaceOptionMarqueeState state;

    state.brightness = (firstScroll & 0x10) != 0 ? 0x80 : 0x40;
    state.firstScroll = AdvanceMarqueeLine(firstScroll);
    state.secondScroll = AdvanceMarqueeLine(secondScroll);
    state.textFrame = sceneTimer & 3;
    return state;
}

RaceOptionPulseState AdvanceRaceOptionPulse(s32 angle) {
    RaceOptionPulseState state;

    state.angle = (s32)(((u32)angle + 0x20) & 0xFFF);
    state.halfWidth = rcos(state.angle) * 0x2C / 4096;
    return state;
}
