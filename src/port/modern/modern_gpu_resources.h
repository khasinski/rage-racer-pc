#ifndef RAGE_MODERN_GPU_RESOURCES_H
#define RAGE_MODERN_GPU_RESOURCES_H

#include <SDL3/SDL_gpu.h>
#include "../port_config.h"

enum { MODERN_GPU_RING_SIZE = 16 };

typedef struct ModernGpuResources {
    int targetW, targetH;
    int bloomW, bloomH;
    int ready;
    unsigned int generation;
    SDL_GPUTexture *target;
    SDL_GPUTexture *depth;
    SDL_GPUSampler *sampler;
    SDL_GPUGraphicsPipeline *pipe3dOpaque;
    SDL_GPUGraphicsPipeline *pipe3dBlend;
    SDL_GPUGraphicsPipeline *pipe3dSub;
    SDL_GPUGraphicsPipeline *pipe2d;
    SDL_GPUGraphicsPipeline *pipe2dSub;
    SDL_GPUBuffer *vertexBuffer;
    SDL_GPUTransferBuffer *vertexTransfer;
    SDL_GPUTexture *postTarget;
    SDL_GPUGraphicsPipeline *pipePost;
    SDL_GPUSampler *samplerLinear;
    SDL_GPUTexture *finalTarget;
    SDL_GPUTexture *bloomA;
    SDL_GPUTexture *bloomB;
    SDL_GPUGraphicsPipeline *pipeBright;
    SDL_GPUGraphicsPipeline *pipeBlurH;
    SDL_GPUGraphicsPipeline *pipeBlurV;
    SDL_GPUGraphicsPipeline *pipeComposite;
    SDL_GPUTexture *ring[MODERN_GPU_RING_SIZE];
} ModernGpuResources;

void ModernGpuResourcesInit(ModernGpuResources *resources);
void ModernGpuResourcesRelease(ModernGpuResources *resources,
                               SDL_GPUDevice *device);
int ModernGpuResourcesCreate(ModernGpuResources *resources,
                             SDL_GPUDevice *device, RagePortConfig *config,
                             float *logicalWidth, float *overscanX,
                             int ringEnabled);

#endif
