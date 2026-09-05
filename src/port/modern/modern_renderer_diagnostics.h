#ifndef RAGE_MODERN_RENDERER_DIAGNOSTICS_H
#define RAGE_MODERN_RENDERER_DIAGNOSTICS_H

#include <SDL3/SDL_gpu.h>
#include "scene_capture.h"
#include "../track_texture_snapshot.h"
#include "render/render_world_snapshot.h"

typedef struct RageModernDiagnosticFrame {
    SDL_GPUDevice *device;
    SDL_GPUTexture *texture;
    /* Borrowed exact texture sampled by the submitted modern overlay frame. */
    SDL_GPUTexture *sampledVram;
    uint32_t sampledVramFrame;
    int width;
    int height;
    float logicalWidth;
    int fps;
    SDL_GPUTexture **ringTextures;
    const unsigned int *ringFrames;
    const float *ringInterpolation;
    const RageSceneSnapshot *ringScenes;
    const RageRenderWorldSnapshot *ringWorlds;
    RageTrackTextureGeneration *const *ringGenerations;
    int ringCount;
    int ringNext;
} RageModernDiagnosticFrame;

void ModernDiagnosticsMaybeDump(
    const RageSceneSnapshot *snapshot,
    const RageModernDiagnosticFrame *frame);
void ModernDiagnosticsCheckMarker(
    const RageSceneSnapshot *snapshot,
    const RageModernDiagnosticFrame *frame,
    int haveModernImage);

#endif
