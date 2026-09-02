#include "game/race_hud_internal.h"
#include "psyq/gte.h"

static s32 AdvanceMarqueeLine(s32 scroll) {
    scroll -= 4;
    return scroll < -624 ? 240 : scroll;
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

    state.angle = (angle + 0x20) & 0xFFF;
    state.halfWidth = rcos(state.angle) * 0x2C / 4096;
    return state;
}
