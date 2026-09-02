#include "game/race_hud_internal.h"

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
    state.textOffset = (sceneTimer & 3) * 40;
    return state;
}
