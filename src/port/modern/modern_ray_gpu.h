#ifndef RAGE_MODERN_RAY_GPU_H
#define RAGE_MODERN_RAY_GPU_H

#include <SDL3/SDL.h>

#include "render/render_mesh_build.h"

int ModernRayGpuInit(SDL_GPUDevice *device);
void ModernRayGpuShutdown(void);
void ModernRayGpuSubmitted(void);
int ModernRayGpuPrepare(SDL_GPUCommandBuffer *command,
                        const RenderWorld *world,
                        RageRenderMeshLookup lookup, void *context,
                        uint64_t assetGeneration);
void ModernRayGpuBind(SDL_GPURenderPass *pass);
uint32_t ModernRayGpuNodeCount(void);
uint32_t ModernRayGpuTriangleCount(void);
uint32_t ModernRayGpuInstanceCount(void);
uint32_t ModernRayGpuUploadBytes(void);
int ModernRayGpuReusedStatic(void);
uint64_t ModernRayGpuBuildNanoseconds(void);

#endif
