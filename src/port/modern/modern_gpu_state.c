#include "modern_gpu_state.h"

#include <string.h>

uint32_t ModernTextureWindowFromGp0(uint32_t word) {
    uint32_t maskX = word & 0x1Fu;
    uint32_t maskY = (word >> 5) & 0x1Fu;
    uint32_t offX = (word >> 10) & 0x1Fu;
    uint32_t offY = (word >> 15) & 0x1Fu;
    if (maskX == 0 && maskY == 0) return 0x0000FFFFu;
    return ((~(maskX * 8) & 0xFFu)) | ((~(maskY * 8) & 0xFFu) << 8) |
           (((offX & maskX) * 8) << 16) | (((offY & maskY) * 8) << 24);
}

void Modern2DStateInit(Modern2DState *state) {
    memset(state, 0, sizeof(*state));
    state->twin = 0x0000FFFFu;
}

void Modern2DStateApplyGp0(Modern2DState *state, uint32_t word,
                           int displayPageY) {
    switch (word >> 24) {
    case 0xE1:
        state->tpage = word & 0x1FFu;
        break;
    case 0xE2:
        state->twin = ModernTextureWindowFromGp0(word);
        break;
    case 0xE3:
        state->scissor.x = (int)(word & 0x3FFu);
        state->areaTopVram = (int)((word >> 10) & 0x1FFu);
        state->hasScissor = 1;
        break;
    case 0xE4: {
        int right = (int)(word & 0x3FFu);
        int bottomVram = (int)((word >> 10) & 0x1FFu);
        int top = state->areaTopVram > displayPageY
                      ? state->areaTopVram : displayPageY;
        int bottom = bottomVram < displayPageY + 239
                         ? bottomVram : displayPageY + 239;
        state->scissor.w = right - state->scissor.x + 1;
        state->scissor.y = top - displayPageY;
        state->scissor.h = bottom - top + 1;
        state->areaEmpty = state->scissor.w <= 0 || state->scissor.h <= 0;
        break;
    }
    case 0xE5: {
        int x = (int)(word & 0x7FFu);
        int y = (int)((word >> 11) & 0x7FFu);
        state->offsetX = (x ^ 1024) - 1024;
        state->offsetY = (y ^ 1024) - 1024;
        break;
    }
    default:
        break;
    }
}
