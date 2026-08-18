#ifndef RAGE_MODERN_GPU_STATE_H
#define RAGE_MODERN_GPU_STATE_H

#include <stdint.h>

typedef struct ModernDrawRect {
    int x, y, w, h;
} ModernDrawRect;

typedef struct Modern2DState {
    uint32_t tpage;
    uint32_t twin;
    ModernDrawRect scissor;
    int areaTopVram;
    int hasScissor;
    int areaEmpty;
    int offsetX, offsetY;
} Modern2DState;

void Modern2DStateInit(Modern2DState *state);
void Modern2DStateApplyGp0(Modern2DState *state, uint32_t word,
                           int displayPageY);
uint32_t ModernTextureWindowFromGp0(uint32_t word);

#endif
