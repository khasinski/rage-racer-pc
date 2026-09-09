#include "modern_overlay_state.h"

#include <assert.h>

int main(void) {
    Modern2DState state;
    SDL_Rect rect;

    ModernOverlayStateInit(&state, 240);
    assert(state.twin == 0x0000FFFFu && state.displayPageY == 240);
    ModernOverlayStateApplyWord(&state, 0xE3000000u | 86u | (240u << 10));
    ModernOverlayStateApplyWord(&state, 0xE4000000u | 233u | (239u << 10));
    assert(state.hasScissor && state.areaEmpty);
    assert(state.scissor.x == 86 && state.scissor.y == 0 &&
           state.scissor.w == 148 && state.scissor.h == 0);

    ModernOverlayStateApplyWord(&state, 0xE3000000u | 10u | (250u << 10));
    ModernOverlayStateApplyWord(&state, 0xE4000000u | 20u | (270u << 10));
    assert(!state.areaEmpty && state.scissor.x == 10 && state.scissor.y == 10 &&
           state.scissor.w == 11 && state.scissor.h == 21);
    ModernOverlayStateApplyWord(&state, 0xE1000000u | 0x1A5u);
    assert(state.tpage == 0x1A5u);
    ModernOverlayStateApplyWord(&state, 0xE2000000u);
    assert(state.twin == 0x0000FFFFu);
    ModernOverlayStateApplyWord(&state, 0xE5000000u | 1024u | (1024u << 11));
    assert(state.offsetX == -1024 && state.offsetY == -1024);

    rect = (SDL_Rect){0, 0, 320, 240};
    ModernOverlayStateScaleScissor(&rect, 320.0f, 0.0f, 640, 480);
    assert(rect.x == 0 && rect.y == 0 && rect.w == 640 && rect.h == 480);
    return 0;
}
