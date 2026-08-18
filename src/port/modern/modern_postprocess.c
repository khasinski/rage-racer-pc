#include "modern_postprocess.h"

static void ModernPostProcessPass(SDL_GPUCommandBuffer *commands,
                                  ModernGpuResources *resources,
                                  SDL_GPUGraphicsPipeline *pipeline,
                                  SDL_GPUTexture *target,
                                  SDL_GPUTexture *sources[], int sourceCount) {
    const SDL_GPUColorTargetInfo color = {
        .texture = target,
        .load_op = SDL_GPU_LOADOP_DONT_CARE,
        .store_op = SDL_GPU_STOREOP_STORE,
    };
    SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(commands, &color, 1, NULL);
    SDL_GPUTextureSamplerBinding bindings[2];
    int index;
    if (pass == NULL) return;
    for (index = 0; index < sourceCount && index < 2; index++) {
        bindings[index].texture = sources[index];
        bindings[index].sampler = resources->samplerLinear;
    }
    SDL_BindGPUGraphicsPipeline(pass, pipeline);
    SDL_BindGPUFragmentSamplers(pass, 0, bindings, (Uint32)sourceCount);
    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
}

SDL_GPUTexture *ModernPostProcessOutput(const ModernGpuResources *resources,
                                        const RagePortConfig *config) {
    if ((config->modernBloom > 0.0f || config->modernGrading) &&
        resources->finalTarget != NULL && resources->pipeComposite != NULL) {
        return resources->finalTarget;
    }
    if (config->modernPost != RAGE_MODERN_POST_NONE &&
        resources->postTarget != NULL) {
        return resources->postTarget;
    }
    return resources->target;
}

void ModernPostProcessRun(SDL_GPUCommandBuffer *commands,
                          ModernGpuResources *resources,
                          const RagePortConfig *config) {
    SDL_GPUTexture *chain = resources->target;
    SDL_GPUTexture *sources[2];

    if (config->modernPost != RAGE_MODERN_POST_NONE &&
        resources->pipePost != NULL && resources->postTarget != NULL) {
        sources[0] = chain;
        ModernPostProcessPass(commands, resources, resources->pipePost,
                              resources->postTarget, sources, 1);
        chain = resources->postTarget;
    }
    if ((config->modernBloom > 0.0f || config->modernGrading) &&
        resources->pipeComposite != NULL && resources->finalTarget != NULL) {
        if (config->modernBloom > 0.0f && resources->pipeBright != NULL) {
            sources[0] = chain;
            ModernPostProcessPass(commands, resources, resources->pipeBright,
                                  resources->bloomA, sources, 1);
            sources[0] = resources->bloomA;
            ModernPostProcessPass(commands, resources, resources->pipeBlurH,
                                  resources->bloomB, sources, 1);
            sources[0] = resources->bloomB;
            ModernPostProcessPass(commands, resources, resources->pipeBlurV,
                                  resources->bloomA, sources, 1);
        }
        sources[0] = chain;
        /* With bloom off kBloom is zero and the second texture is unused. */
        sources[1] = resources->bloomA != NULL ? resources->bloomA : chain;
        ModernPostProcessPass(commands, resources, resources->pipeComposite,
                              resources->finalTarget, sources, 2);
    }
}
