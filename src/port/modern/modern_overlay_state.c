#include "modern_overlay_state.h"

#include <string.h>

static uint32_t TwinFromE2(uint32_t word) {
    uint32_t maskX = word & 0x1Fu;
    uint32_t maskY = (word >> 5) & 0x1Fu;
    uint32_t offX = (word >> 10) & 0x1Fu;
    uint32_t offY = (word >> 15) & 0x1Fu;
    if (maskX == 0 && maskY == 0) return 0x0000FFFFu;
    return ((~(maskX * 8) & 0xFFu)) | ((~(maskY * 8) & 0xFFu) << 8) |
           (((offX & maskX) * 8) << 16) | (((offY & maskY) * 8) << 24);
}

void ModernOverlayStateInit(Modern2DState *state, int displayPageY) {
    if (!state) return;
    memset(state, 0, sizeof(*state));
    state->twin = 0x0000FFFFu;
    state->displayPageY = displayPageY;
    state->areaTopVram = displayPageY;
}

void ModernOverlayStateApplyWord(Modern2DState *state, uint32_t word) {
    int x, y, top, bottom;
    if (!state) return;
    switch (word >> 24) {
    case 0xE1:
        state->tpage = word & 0x1FFu;
        break;
    case 0xE2:
        state->twin = TwinFromE2(word);
        break;
    case 0xE3:
        state->scissor.x = (int)(word & 0x3FFu);
        state->areaTopVram = (int)((word >> 10) & 0x1FFu);
        state->hasScissor = 1;
        break;
    case 0xE4:
        /* Drawing-area rows are VRAM-absolute. Keep a zero-height area empty:
         * mirror slides deliberately emit (top=pageY, bottom=pageY-1), and
         * folding that interval into a full page causes a visible flash. */
        x = (int)(word & 0x3FFu);
        y = (int)((word >> 10) & 0x1FFu);
        top = state->areaTopVram > state->displayPageY
            ? state->areaTopVram : state->displayPageY;
        bottom = y < state->displayPageY + 239 ? y : state->displayPageY + 239;
        state->scissor.w = x - state->scissor.x + 1;
        state->scissor.y = top - state->displayPageY;
        state->scissor.h = bottom - top + 1;
        state->areaEmpty = state->scissor.w <= 0 || state->scissor.h <= 0;
        break;
    case 0xE5:
        x = (int)(word & 0x7FFu);
        y = (int)((word >> 11) & 0x7FFu);
        state->offsetX = (x ^ 1024) - 1024;
        state->offsetY = (y ^ 1024) - 1024;
        break;
    default:
        break;
    }
}

void ModernOverlayStateScaleScissor(SDL_Rect *rect, float logicalWidth,
                                    float overscanX, int targetWidth,
                                    int targetHeight) {
    float scale;
    if (!rect || logicalWidth <= 0 || targetWidth <= 0 || targetHeight <= 0)
        return;
    scale = (float)targetWidth / logicalWidth;
    rect->x = (int)(((float)rect->x + overscanX) * scale);
    rect->y = rect->y * targetHeight / 240;
    rect->w = (int)((float)rect->w * scale);
    rect->h = rect->h * targetHeight / 240;
}
