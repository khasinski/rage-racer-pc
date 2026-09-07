#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include "modern/modern_vram_snapshot.h"
#define BYTES (1024u * 512u * 4u)
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s: %s\n", \
    __LINE__, #x, SDL_GetError()); return 1; } } while (0)
static int Upload(SDL_GPUDevice *device, SDL_GPUTexture *texture,
                  SDL_GPUTransferBuffer *transfer, unsigned char value) {
    void *map = SDL_MapGPUTransferBuffer(device, transfer, true);
    CHECK(map);
    memset(map, value, BYTES);
    SDL_UnmapGPUTransferBuffer(device, transfer);
    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(device);
    CHECK(cmd);
    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
    CHECK(copy);
    SDL_GPUTextureTransferInfo from = {.transfer_buffer = transfer,
        .pixels_per_row = 1024, .rows_per_layer = 512};
    SDL_GPUTextureRegion to = {.texture = texture, .w = 1024, .h = 512, .d = 1};
    SDL_UploadToGPUTexture(copy, &from, &to, false);
    SDL_EndGPUCopyPass(copy);
    CHECK(SDL_SubmitGPUCommandBuffer(cmd));
    return 0;
}
static int Verify(SDL_GPUDevice *device, SDL_GPUTexture *texture,
                  SDL_GPUTransferBuffer *transfer, unsigned char value) {
    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(device);
    CHECK(cmd);
    SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
    CHECK(copy);
    SDL_GPUTextureRegion from = {.texture = texture, .w = 1024, .h = 512, .d = 1};
    SDL_GPUTextureTransferInfo to = {.transfer_buffer = transfer,
        .pixels_per_row = 1024, .rows_per_layer = 512};
    SDL_DownloadFromGPUTexture(copy, &from, &to);
    SDL_EndGPUCopyPass(copy);
    SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
    CHECK(fence);
    CHECK(SDL_WaitForGPUFences(device, true, &fence, 1));
    SDL_ReleaseGPUFence(device, fence);
    unsigned char *map = SDL_MapGPUTransferBuffer(device, transfer, false);
    CHECK(map);
    for (size_t i = 0; i < BYTES; ++i) CHECK(map[i] == value);
    SDL_UnmapGPUTransferBuffer(device, transfer);
    return 0;
}
int main(void) {
    if (!SDL_Init(SDL_INIT_VIDEO)) return 77;
    SDL_GPUDevice *device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, false, NULL);
    if (!device) { SDL_Quit(); return 77; }
    SDL_GPUTextureCreateInfo info = {.type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER, .width = 1024, .height = 512,
        .layer_count_or_depth = 1, .num_levels = 1};
    SDL_GPUTexture *source = SDL_CreateGPUTexture(device, &info), *owned = NULL;
    CHECK(source);
    SDL_GPUTransferBufferCreateInfo transfer = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD, .size = BYTES};
    SDL_GPUTransferBuffer *upload = SDL_CreateGPUTransferBuffer(device, &transfer);
    transfer.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
    SDL_GPUTransferBuffer *download = SDL_CreateGPUTransferBuffer(device, &transfer);
    CHECK(upload && download);
    CHECK(!ModernVramSnapshotCopy(device, NULL, &owned) && !owned);
    CHECK(!Upload(device, source, upload, 0x31));
    CHECK(ModernVramSnapshotCopy(device, source, &owned));
    CHECK(owned && owned != source);
    CHECK(!Upload(device, source, upload, 0xd2));
    CHECK(!Verify(device, source, download, 0xd2));
    CHECK(!Verify(device, owned, download, 0x31));
    CHECK(!ModernVramSnapshotCopy(NULL, source, &owned));
    CHECK(!Verify(device, owned, download, 0x31));
    CHECK(ModernVramSnapshotCopy(device, source, &owned));
    CHECK(!Verify(device, owned, download, 0xd2));
    SDL_ReleaseGPUTexture(device, owned);
    owned = NULL;
    CHECK(ModernVramSnapshotCopy(device, source, &owned));
    CHECK(!Verify(device, owned, download, 0xd2));
    SDL_ReleaseGPUTexture(device, owned);
    SDL_ReleaseGPUTexture(device, source);
    SDL_ReleaseGPUTransferBuffer(device, upload);
    SDL_ReleaseGPUTransferBuffer(device, download);
    SDL_DestroyGPUDevice(device);
    SDL_Quit();
    puts("Owned VRAM snapshot survives source mutation and recreation");
    return 0;
}
