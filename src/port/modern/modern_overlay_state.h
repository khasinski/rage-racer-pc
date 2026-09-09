#ifndef RAGE_MODERN_OVERLAY_STATE_H
#define RAGE_MODERN_OVERLAY_STATE_H

#include <SDL3/SDL.h>

#include <stdint.h>

/* The GP0 environment state belongs to a captured packet stream, independently
 * of the GPU vertex buffer that eventually consumes it. */
typedef struct Modern2DState {
    uint32_t tpage;
    uint32_t twin;
    SDL_Rect scissor;
    int displayPageY;
    int areaTopVram;
    int hasScissor;
    int areaEmpty;
    int offsetX;
    int offsetY;
} Modern2DState;

void ModernOverlayStateInit(Modern2DState *state, int displayPageY);
void ModernOverlayStateApplyWord(Modern2DState *state, uint32_t word);
void ModernOverlayStateScaleScissor(SDL_Rect *rect, float logicalWidth,
                                    float overscanX, int targetWidth,
                                    int targetHeight);

#endif
