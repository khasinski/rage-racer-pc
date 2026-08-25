#ifndef RAGE_MODERN_NATIVE_GPU_H
#define RAGE_MODERN_NATIVE_GPU_H

#include <SDL3/SDL.h>

#include "render/render_world.h"

int ModernNativeGpuInit(SDL_GPUDevice *device);
void ModernNativeGpuShutdown(void);
void ModernNativeGpuPrepare(const RageRenderWorld *world, float aspect);
int ModernNativeGpuHasDraws(void);
int ModernNativeGpuCanReplaceWorld(void);
void ModernNativeGpuDraw(SDL_GPUCommandBuffer *command,
                         SDL_GPUTexture *vram,
                         SDL_GPUTexture *colorTarget,
                         SDL_GPUTexture *depthTarget,
                         int clearColor);

#endif
