#ifndef RAGE_MODERN_RENDERER_DIAGNOSTICS_H
#define RAGE_MODERN_RENDERER_DIAGNOSTICS_H

#include <SDL3/SDL_gpu.h>
#include "scene_capture.h"

typedef struct RageModernDiagnosticFrame {
    SDL_GPUDevice *device;
    SDL_GPUTexture *texture;
    int width;
    int height;
    float logicalWidth;
    int fps;
    SDL_GPUTexture **ringTextures;
    const unsigned int *ringFrames;
    const float *ringInterpolation;
    const RageSceneSnapshot *ringScenes;
    int ringCount;
    int ringNext;
} RageModernDiagnosticFrame;

void RageModernDiagnosticsMaybeDump(
    const RageSceneSnapshot *snapshot,
    const RageModernDiagnosticFrame *frame);
void RageModernDiagnosticsCheckMarker(
    const RageSceneSnapshot *snapshot,
    const RageModernDiagnosticFrame *frame,
    int haveModernImage);

#endif
