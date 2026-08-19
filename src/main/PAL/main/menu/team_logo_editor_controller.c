#include "game/team_logo_editor_controller.h"

static s32 MoveWrapped(s32 value, s32 minimum, s32 maximum, s32 direction) {
    value += direction < 0 ? -1 : 1;
    if (value < minimum) return maximum;
    if (value > maximum) return minimum;
    return value;
}

s32 TeamLogoCycleGuideMode(s32 guideMode) {
    return MoveWrapped(guideMode, 0, 2, 1);
}

s32 TeamLogoMovePaletteSlot(s32 slot, s32 direction) {
    return MoveWrapped(slot, 1, 15, direction);
}

s32 TeamLogoMoveColorChannel(s32 channel, s32 direction) {
    return MoveWrapped(channel, 0, 2, direction);
}
