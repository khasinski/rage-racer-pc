#ifndef RAGE_MODERN_RAY_GPU_H
#define RAGE_MODERN_RAY_GPU_H

#include <SDL3/SDL.h>

#include "render/render_native_vertex.h"
#include "render/ray/ray_draws.h"

int ModernRayGpuInit(SDL_GPUDevice *device);
void ModernRayGpuShutdown(void);
void ModernRayGpuSubmitted(void);
int ModernRayGpuPrepare(SDL_GPUCommandBuffer *command,
                        const RageNativeGpuVertex *vertices,
                        uint32_t vertexCount,
                        const RageNativeDrawSpan *spans,
                        uint32_t spanCount, RayDrawInclude include,
                        void *context);
void ModernRayGpuBind(SDL_GPURenderPass *pass);
uint32_t ModernRayGpuNodeCount(void);
uint32_t ModernRayGpuTriangleCount(void);
uint64_t ModernRayGpuBuildNanoseconds(void);

#endif
