#ifndef RAGE_MODERN_NATIVE_GPU_H
#define RAGE_MODERN_NATIVE_GPU_H

#include <SDL3/SDL.h>
#include <stdio.h>

#include "render/render_world.h"

int ModernNativeGpuInit(SDL_GPUDevice *device);
void ModernNativeGpuShutdown(void);
/* world->frame is a presentation revision, not necessarily a simulation tick.
 * Callers must advance it when camera/instance/environment contents change.
 * Preparation may reuse an unchanged revision and aspect within the same
 * asset generation; aspect changes are detected independently. The game
 * interpolation adapter issues a fresh revision for each presentation. */
void ModernNativeGpuPrepare(const RageRenderWorld *world, float aspect);
/* Call after successfully submitting the command buffer used by Draw and
 * DrawMirror. On cancellation/submission failure, shut down this renderer
 * before reuse: cached textures and geometry may refer to discarded uploads. */
void ModernNativeGpuSubmitted(void);
/* Backend-owned immutable values, borrowed until the next preparation or
 * shutdown. Mesh/material IDs still refer to external asset generations. */
const RageRenderWorld *ModernNativeGpuPreparedWorld(void);
uint64_t ModernNativeGpuTextureRevision(void);
int ModernNativeGpuWriteDrawDump(FILE *file);
/* Diagnostic CPU-only measurement of the resident world; restores its revision.
 * Invoke between submitted frames, with performance/asset tracing disabled. */
int ModernNativeGpuBenchmarkPrepare(FILE *file, unsigned repeats);
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
