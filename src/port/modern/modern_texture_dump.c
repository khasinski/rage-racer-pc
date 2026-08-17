#include "modern_texture_dump.h"

#include <stdint.h>
#include <stdio.h>

int RageModernWriteTexturePpm(SDL_GPUDevice *device, SDL_GPUTexture *texture,
                              int width, int height, const char *path) {
    SDL_GPUTransferBufferCreateInfo info = {0};
    SDL_GPUTransferBuffer *transfer;
    SDL_GPUCommandBuffer *commands;
    int written = 0;

    if (device == NULL || texture == NULL || width <= 0 || height <= 0 ||
        path == NULL || path[0] == '\0')
        return 0;
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    info.size = (Uint32)(width * height * 4);
    transfer = SDL_CreateGPUTransferBuffer(device, &info);
    if (transfer == NULL) return 0;
    commands = SDL_AcquireGPUCommandBuffer(device);
    if (commands != NULL) {
        SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(commands);
        const SDL_GPUTextureRegion region = {
            .texture = texture,
            .w = (Uint32)width,
            .h = (Uint32)height,
            .d = 1,
        };
        const SDL_GPUTextureTransferInfo destination = {
            .transfer_buffer = transfer,
        };
        SDL_DownloadFromGPUTexture(copy, &region, &destination);
        SDL_EndGPUCopyPass(copy);
        {
            SDL_GPUFence *fence =
                SDL_SubmitGPUCommandBufferAndAcquireFence(commands);
            if (fence != NULL) {
                SDL_WaitForGPUFences(device, true, &fence, 1);
                SDL_ReleaseGPUFence(device, fence);
            }
        }
        {
            const uint8_t *pixels =
                SDL_MapGPUTransferBuffer(device, transfer, false);
            if (pixels != NULL) {
                FILE *file = fopen(path, "wb");
                if (file != NULL) {
                    int pixel;
                    fprintf(file, "P6\n%d %d\n255\n", width, height);
                    for (pixel = 0; pixel < width * height; pixel++)
                        fwrite(pixels + pixel * 4, 1, 3, file);
                    written = fclose(file) == 0;
                }
                SDL_UnmapGPUTransferBuffer(device, transfer);
            }
        }
    }
    SDL_ReleaseGPUTransferBuffer(device, transfer);
    return written;
}
