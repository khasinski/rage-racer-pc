#ifndef RAGE_MODERN_NATIVE_GPU_H
#define RAGE_MODERN_NATIVE_GPU_H

#include <SDL3/SDL.h>
#include <stdio.h>

#include "render/render_world.h"

int ModernNativeGpuInit(SDL_GPUDevice *device);
void ModernNativeGpuShutdown(void);
void ModernNativeGpuPrepare(const RageRenderWorld *world, float aspect);
/* Call after successfully submitting the command buffer used by Draw and
 * DrawMirror. On cancellation/submission failure, shut down this renderer
 * before reuse: cached textures may refer to discarded uploads. */
void ModernNativeGpuSubmitted(void);
const RageRenderWorld *ModernNativeGpuPreparedWorld(void);
uint64_t ModernNativeGpuTextureRevision(void);
int ModernNativeGpuWriteDrawDump(FILE *file);
int ModernNativeGpuWriteProbe(FILE *file, int x, int y,
                              int width, int height);
int ModernNativeGpuHasDraws(void);
int ModernNativeGpuWorldComplete(void);
int ModernNativeGpuHasMirrorDraws(void);
float ModernNativeGpuMirrorPanelY(void);
void ModernNativeGpuDraw(SDL_GPUCommandBuffer *command,
                         SDL_GPUTexture *colorTarget,
                         SDL_GPUTexture *depthTarget,
                         int clearColor, int drawSky, int targetHeight);
void ModernNativeGpuDrawMirror(SDL_GPUCommandBuffer *command,
                               SDL_GPUTexture *colorTarget,
                               SDL_GPUTexture *depthTarget, int targetHeight);

#endif
