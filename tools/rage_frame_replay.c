#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "port/modern/modern_assets.h"
#include "port/modern/modern_native_gpu.h"
#include "port/modern/modern_texture_dump.h"
#include "port/runtime_config.h"
#include "render/render_world_snapshot.h"

/* The legacy archive override provider shares one translation unit with the
 * semantic mod root queried by ModernAssets. Frame replay never loads the
 * retail archive, so it has no PS1 asset arena to measure. */
size_t RagePortAssetRoomAt(const void *at) {
    (void)at;
    return 0;
}

static const char *OptionValue(int argc, char **argv, const char *option) {
    int index;
    for (index = 1; index + 1 < argc; index++)
        if (strcmp(argv[index], option) == 0) return argv[index + 1];
    return NULL;
}

static int ParseDimension(const char *text, int fallback) {
    char *end;
    long value;
    if (text == NULL) return fallback;
    value = strtol(text, &end, 10);
    return end != text && *end == '\0' && value >= 16 && value <= 16384
               ? (int)value
               : 0;
}

static void Usage(const char *program) {
    fprintf(stderr,
            "usage: %s FRAME.world.bin --assets NATIVE_ASSETS "
            "[--output FRAME.ppm] [--draws FRAME.draws.txt] "
            "[--width 1280] [--height 960]\n",
            program);
}

int main(int argc, char **argv) {
    const char *snapshotPath;
    const char *outputPath;
    const char *drawPath;
    const char *assetsPath;
    RageRenderWorldSnapshot snapshot;
    SDL_GPUDevice *device = NULL;
    SDL_GPUTexture *color = NULL;
    SDL_GPUTexture *depth = NULL;
    SDL_GPUCommandBuffer *command;
    int width, height;
    int result = EXIT_FAILURE;

    if (argc < 2 || argv[1][0] == '-') {
        Usage(argc > 0 ? argv[0] : "rage-frame-replay");
        return EXIT_FAILURE;
    }
    snapshotPath = argv[1];
    outputPath = OptionValue(argc, argv, "--output");
    drawPath = OptionValue(argc, argv, "--draws");
    assetsPath = OptionValue(argc, argv, "--assets");
    width = ParseDimension(OptionValue(argc, argv, "--width"), 1280);
    height = ParseDimension(OptionValue(argc, argv, "--height"), 960);
    if (width == 0 || height == 0 || assetsPath == NULL) {
        Usage(argv[0]);
        return EXIT_FAILURE;
    }
    if (outputPath == NULL) outputPath = "frame-replay.ppm";

    if (!RageRuntimeConfigInit(argc, argv)) goto cleanup;
    if (!RageRenderWorldSnapshotRead(snapshotPath, &snapshot)) {
        fprintf(stderr, "rage-frame-replay: cannot read %s\n", snapshotPath);
        goto cleanup;
    }
    /* The ordinary runtime override keeps asset selection identical between
     * the game and the harness without adding a second asset-loader path. */
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "rage-frame-replay: SDL_Init: %s\n", SDL_GetError());
        goto release_snapshot;
    }
    device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, true, NULL);
    if (device == NULL) {
        fprintf(stderr, "rage-frame-replay: GPU: %s\n", SDL_GetError());
        goto release_snapshot;
    }
    if (!ModernAssetsInitRoot(assetsPath) || !ModernNativeGpuInit(device))
        goto release_gpu;
    {
        SDL_GPUTextureCreateInfo info = {0};
        info.type = SDL_GPU_TEXTURETYPE_2D;
        info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
        info.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET |
                     SDL_GPU_TEXTUREUSAGE_SAMPLER;
        info.width = (Uint32)width;
        info.height = (Uint32)height;
        info.layer_count_or_depth = 1;
        info.num_levels = 1;
        color = SDL_CreateGPUTexture(device, &info);
        info.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
        depth = SDL_CreateGPUTexture(device, &info);
    }
    if (color == NULL || depth == NULL) {
        fprintf(stderr, "rage-frame-replay: target: %s\n", SDL_GetError());
        goto release_renderer;
    }
    ModernNativeGpuPrepare(&snapshot.world, (float)width / (float)height);
    if (!ModernNativeGpuHasDraws()) {
        fprintf(stderr, "rage-frame-replay: snapshot produced no draws\n");
        goto release_renderer;
    }
    command = SDL_AcquireGPUCommandBuffer(device);
    if (command == NULL) {
        fprintf(stderr, "rage-frame-replay: command buffer: %s\n",
                SDL_GetError());
        goto release_renderer;
    }
    ModernNativeGpuDraw(command, color, depth, 1);
    if (!SDL_SubmitGPUCommandBuffer(command)) {
        fprintf(stderr, "rage-frame-replay: submit: %s\n", SDL_GetError());
        goto release_renderer;
    }
    if (!RageModernWriteTexturePpm(device, color, width, height, outputPath)) {
        fprintf(stderr, "rage-frame-replay: cannot write %s\n", outputPath);
        goto release_renderer;
    }
    if (drawPath != NULL) {
        FILE *file = fopen(drawPath, "w");
        if (file == NULL || !ModernNativeGpuWriteDrawDump(file)) {
            if (file != NULL) fclose(file);
            fprintf(stderr, "rage-frame-replay: cannot write %s\n", drawPath);
            goto release_renderer;
        }
        fclose(file);
    }
    fprintf(stderr,
            "rage-frame-replay: frame=%llu instances=%u output=%s\n",
            (unsigned long long)snapshot.world.frame,
            snapshot.world.instanceCount, outputPath);
    result = EXIT_SUCCESS;

release_renderer:
    if (color != NULL) SDL_ReleaseGPUTexture(device, color);
    if (depth != NULL) SDL_ReleaseGPUTexture(device, depth);
    ModernNativeGpuShutdown();
    ModernAssetsShutdown();
release_gpu:
    SDL_DestroyGPUDevice(device);
release_snapshot:
    SDL_Quit();
    RageRenderWorldSnapshotRelease(&snapshot);
cleanup:
    return result;
}
