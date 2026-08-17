#ifndef RAGE_MODERN_TEXTURE_DUMP_H
#define RAGE_MODERN_TEXTURE_DUMP_H

#include <SDL3/SDL_gpu.h>

int RageModernWriteTexturePpm(SDL_GPUDevice *device, SDL_GPUTexture *texture,
                              int width, int height, const char *path);

#endif
