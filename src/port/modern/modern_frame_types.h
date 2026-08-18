#ifndef RAGE_MODERN_FRAME_TYPES_H
#define RAGE_MODERN_FRAME_TYPES_H

#include <SDL3/SDL_rect.h>
#include <stdint.h>

typedef struct ModernVertex {
    float x, y, z, w;
    float u, v;
    uint8_t color[4];
    uint32_t attr;
    uint32_t twin;
    uint32_t clut;
} ModernVertex;

typedef enum ModernPipelineId {
    MODERN_PIPE_3D_OPAQUE,
    MODERN_PIPE_3D_BLEND,
    MODERN_PIPE_3D_SUB,
    MODERN_PIPE_2D,
    MODERN_PIPE_2D_SUB,
    MODERN_PIPE_COUNT
} ModernPipelineId;

typedef struct ModernSpan {
    uint8_t pipeline;
    uint8_t hasScissor;
    uint8_t pass;
    SDL_Rect scissor;
    int32_t start, count;
    float depthKey;
} ModernSpan;

enum {
    MODERN_MAX_VERTICES = 400000,
    MODERN_MAX_SPANS = 16384
};

#endif
