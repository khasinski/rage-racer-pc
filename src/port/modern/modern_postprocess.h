#ifndef RAGE_MODERN_POSTPROCESS_H
#define RAGE_MODERN_POSTPROCESS_H

#include "modern_gpu_resources.h"
#include "../port_config.h"

SDL_GPUTexture *ModernPostProcessOutput(const ModernGpuResources *resources,
                                        const RagePortConfig *config);
void ModernPostProcessRun(SDL_GPUCommandBuffer *commands,
                          ModernGpuResources *resources,
                          const RagePortConfig *config);

#endif
