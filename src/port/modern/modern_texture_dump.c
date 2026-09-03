#include "modern_texture_dump.h"

#include <stdint.h>
#include <stdio.h>

int ModernWriteTexturePpm(SDL_GPUDevice *device, SDL_GPUTexture *texture,
                          int width, int height, const char *path) {
    SDL_GPUTransferBufferCreateInfo info = {0};
    SDL_GPUTransferBuffer *transfer;
    SDL_GPUCommandBuffer *commands;
    SDL_GPUCopyPass *copy;
    SDL_GPUFence *fence;
    const uint8_t *pixels;
    size_t pixelCount;
    size_t pixel;
    int written = 0;

    if (device == NULL || texture == NULL || width <= 0 || height <= 0 ||
        path == NULL || path[0] == '\0')
        return 0;
    if ((size_t)width > SIZE_MAX / (size_t)height) return 0;
    pixelCount = (size_t)width * (size_t)height;
    if (pixelCount > UINT32_MAX / 4u) return 0;
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    info.size = (Uint32)(pixelCount * 4u);
    transfer = SDL_CreateGPUTransferBuffer(device, &info);
    if (transfer == NULL) return 0;
    commands = SDL_AcquireGPUCommandBuffer(device);
    if (commands != NULL) {
        const SDL_GPUTextureRegion region = {
            .texture = texture,
            .w = (Uint32)width,
            .h = (Uint32)height,
            .d = 1,
        };
        const SDL_GPUTextureTransferInfo destination = {
            .transfer_buffer = transfer,
        };
        copy = SDL_BeginGPUCopyPass(commands);
        if (copy == NULL) {
            SDL_CancelGPUCommandBuffer(commands);
            SDL_ReleaseGPUTransferBuffer(device, transfer);
            return 0;
        }
        SDL_DownloadFromGPUTexture(copy, &region, &destination);
        SDL_EndGPUCopyPass(copy);
        fence = SDL_SubmitGPUCommandBufferAndAcquireFence(commands);
        if (fence != NULL) {
            FILE *file;
            SDL_WaitForGPUFences(device, true, &fence, 1);
            SDL_ReleaseGPUFence(device, fence);
            pixels = SDL_MapGPUTransferBuffer(device, transfer, false);
            if (pixels != NULL) {
                file = fopen(path, "wb");
                if (file != NULL) {
                    written = fprintf(file, "P6\n%d %d\n255\n", width,
                                      height) > 0;
                    for (pixel = 0; written && pixel < pixelCount; pixel++)
                        written = fwrite(pixels + pixel * 4u, 1, 3, file) == 3;
                    if (fclose(file) != 0) written = 0;
                }
                SDL_UnmapGPUTransferBuffer(device, transfer);
            }
        }
    }
    SDL_ReleaseGPUTransferBuffer(device, transfer);
    return written;
}
