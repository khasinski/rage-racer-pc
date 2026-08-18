#include "modern_gpu_resources.h"

#include <string.h>

void ModernGpuResourcesInit(ModernGpuResources *resources) {
    memset(resources, 0, sizeof(*resources));
}

void ModernGpuResourcesRelease(ModernGpuResources *resources,
                               SDL_GPUDevice *device) {
    unsigned int generation = resources->generation;
    int slot;

    if (device != NULL) {
#define RAGE_RELEASE(kind, value) do {                                        \
        if ((value) != NULL) SDL_ReleaseGPU##kind(device, (value));           \
    } while (0)
        RAGE_RELEASE(Texture, resources->target);
        RAGE_RELEASE(Texture, resources->depth);
        RAGE_RELEASE(Sampler, resources->sampler);
        RAGE_RELEASE(GraphicsPipeline, resources->pipe3dOpaque);
        RAGE_RELEASE(GraphicsPipeline, resources->pipe3dBlend);
        RAGE_RELEASE(GraphicsPipeline, resources->pipe3dSub);
        RAGE_RELEASE(GraphicsPipeline, resources->pipe2d);
        RAGE_RELEASE(GraphicsPipeline, resources->pipe2dSub);
        RAGE_RELEASE(Buffer, resources->vertexBuffer);
        RAGE_RELEASE(TransferBuffer, resources->vertexTransfer);
        RAGE_RELEASE(Texture, resources->postTarget);
        RAGE_RELEASE(GraphicsPipeline, resources->pipePost);
        RAGE_RELEASE(Sampler, resources->samplerLinear);
        RAGE_RELEASE(Texture, resources->finalTarget);
        RAGE_RELEASE(Texture, resources->bloomA);
        RAGE_RELEASE(Texture, resources->bloomB);
        RAGE_RELEASE(GraphicsPipeline, resources->pipeBright);
        RAGE_RELEASE(GraphicsPipeline, resources->pipeBlurH);
        RAGE_RELEASE(GraphicsPipeline, resources->pipeBlurV);
        RAGE_RELEASE(GraphicsPipeline, resources->pipeComposite);
        for (slot = 0; slot < MODERN_GPU_RING_SIZE; slot++)
            RAGE_RELEASE(Texture, resources->ring[slot]);
#undef RAGE_RELEASE
    }
    memset(resources, 0, sizeof(*resources));
    resources->generation = generation;
}
