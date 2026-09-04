#include <SDL3/SDL.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "port/modern/scene_capture.h"
#include "port/modern/modern_assets.h"
#include "port/modern/modern_native_gpu.h"
#include "port/modern/modern_texture_dump.h"
#include "port/runtime_config.h"
#include "render/render_world_snapshot.h"

/* The legacy archive override provider shares one translation unit with the
 * semantic mod root queried by ModernAssets. Frame replay never loads the
 * retail archive, so it has no PS1 asset arena to measure. */
size_t PortAssetRoomAt(const void *at) {
    (void)at;
    return 0;
}

static const char *OptionValue(int argc, char **argv, const char *option) {
    int index;
    for (index = 1; index + 1 < argc; index++)
        if (strcmp(argv[index], option) == 0) return argv[index + 1];
    return NULL;
}

static int HasOption(int argc, char **argv, const char *option) {
    int index;
    for (index = 1; index < argc; index++)
        if (strcmp(argv[index], option) == 0) return 1;
    return 0;
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
            "[--camera-scene MARKER.scene.bin] [--sky-only] "
            "[--probe X,Y] "
            "[--sky top|middle|horizon|bottom=R,G,B] "
            "[--width 1280] [--height 960]\n",
            program);
}

static RageRenderQuaternion QuaternionFromMatrix(float matrix[3][3]) {
    RageRenderQuaternion out;
    float trace = matrix[0][0] + matrix[1][1] + matrix[2][2];
    float root;
    if (trace > 0.0f) {
        root = sqrtf(trace + 1.0f) * 2.0f;
        out.w = 0.25f * root;
        out.x = (matrix[2][1] - matrix[1][2]) / root;
        out.y = (matrix[0][2] - matrix[2][0]) / root;
        out.z = (matrix[1][0] - matrix[0][1]) / root;
    } else if (matrix[0][0] > matrix[1][1] &&
               matrix[0][0] > matrix[2][2]) {
        root = sqrtf(1.0f + matrix[0][0] - matrix[1][1] - matrix[2][2]) * 2.0f;
        out.w = (matrix[2][1] - matrix[1][2]) / root;
        out.x = 0.25f * root;
        out.y = (matrix[0][1] + matrix[1][0]) / root;
        out.z = (matrix[0][2] + matrix[2][0]) / root;
    } else if (matrix[1][1] > matrix[2][2]) {
        root = sqrtf(1.0f + matrix[1][1] - matrix[0][0] - matrix[2][2]) * 2.0f;
        out.w = (matrix[0][2] - matrix[2][0]) / root;
        out.x = (matrix[0][1] + matrix[1][0]) / root;
        out.y = 0.25f * root;
        out.z = (matrix[1][2] + matrix[2][1]) / root;
    } else {
        root = sqrtf(1.0f + matrix[2][2] - matrix[0][0] - matrix[1][1]) * 2.0f;
        out.w = (matrix[1][0] - matrix[0][1]) / root;
        out.x = (matrix[0][2] + matrix[2][0]) / root;
        out.y = (matrix[1][2] + matrix[2][1]) / root;
        out.z = 0.25f * root;
    }
    return out;
}

static int ApplyCapturedCamera(const char *path, RageRenderWorld *world) {
    RageSceneSnapshot *scene;
    FILE *file;
    float source[3][3], converted[3][3], pose[3][3];
    int row, column;
    int ok;
    if (path == NULL) return 1;
    scene = malloc(sizeof(*scene));
    if (scene == NULL) return 0;
    file = fopen(path, "rb");
    ok = file != NULL && fread(scene, sizeof(*scene), 1, file) == 1;
    if (file != NULL) fclose(file);
    if (!ok) {
        fprintf(stderr, "rage-frame-replay: cannot read camera scene %s\n",
                path);
        free(scene);
        return 0;
    }
    for (row = 0; row < 3; row++)
        for (column = 0; column < 3; column++)
            source[row][column] =
                (float)scene->viewMatrix.m[row][column] / 4096.0f;
    RenderConvertPsxMatrix(source, converted);
    for (row = 0; row < 3; row++)
        for (column = 0; column < 3; column++)
            pose[row][column] = converted[column][row];
    world->camera.transform.position.x = (float)scene->viewPosition[0];
    world->camera.transform.position.y = -(float)scene->viewPosition[1];
    world->camera.transform.position.z = -(float)scene->viewPosition[2];
    world->camera.transform.orientation = QuaternionFromMatrix(pose);
    world->camera.transform.hasOrientation = 1;
    world->previousCamera = world->camera;
    world->hasCamera = 1;
    fprintf(stderr,
            "rage-frame-replay: camera scene frame=%u pos=%d,%d,%d\n",
            scene->frameCounter, scene->viewPosition[0],
            scene->viewPosition[1], scene->viewPosition[2]);
    free(scene);
    return 1;
}

/*
 * The snapshot stores the sky as four finished colours, so a change to which
 * environment slot feeds which band cannot be tried against a recorded frame
 * without replaying the game. Let the harness substitute them directly: the
 * mapping is what one iterates on, and it is the one thing the capture bakes.
 */
static int ApplySkyOverrides(int argc, char **argv, RageRenderCamera *camera) {
    static const struct {
        const char *name;
        size_t offset;
    } bands[] = {
        {"top", offsetof(RageRenderCamera, skyTopColor)},
        {"middle", offsetof(RageRenderCamera, skyColor)},
        {"horizon", offsetof(RageRenderCamera, skyHorizonColor)},
        {"bottom", offsetof(RageRenderCamera, skyBottomColor)},
    };
    int index;
    for (index = 1; index < argc; index++) {
        const char *value;
        size_t band;
        if (strcmp(argv[index], "--sky") != 0) continue;
        value = index + 1 < argc ? argv[index + 1] : NULL;
        if (value == NULL) {
            fprintf(stderr, "rage-frame-replay: --sky needs BAND=R,G,B\n");
            return 0;
        }
        for (band = 0; band < sizeof(bands) / sizeof(bands[0]); band++) {
            size_t length = strlen(bands[band].name);
            RageRenderVec3 *target;
            unsigned red, green, blue;
            if (strncmp(value, bands[band].name, length) != 0 ||
                value[length] != '=')
                continue;
            if (sscanf(value + length + 1, "%u,%u,%u", &red, &green,
                       &blue) != 3) {
                fprintf(stderr, "rage-frame-replay: bad --sky %s\n", value);
                return 0;
            }
            target = (RageRenderVec3 *)((char *)camera + bands[band].offset);
            target->x = (float)red / 255.0f;
            target->y = (float)green / 255.0f;
            target->z = (float)blue / 255.0f;
            break;
        }
        if (band == sizeof(bands) / sizeof(bands[0])) {
            fprintf(stderr, "rage-frame-replay: unknown sky band %s\n", value);
            return 0;
        }
    }
    return 1;
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

    if (!RuntimeConfigInit(argc, argv)) goto cleanup;
    if (!RenderWorldSnapshotRead(snapshotPath, &snapshot)) {
        fprintf(stderr, "rage-frame-replay: cannot read %s\n", snapshotPath);
        goto cleanup;
    }
    if (!ApplyCapturedCamera(OptionValue(argc, argv, "--camera-scene"),
                             &snapshot.world))
        goto release_snapshot;
    if (!ApplySkyOverrides(argc, argv, &snapshot.world.camera))
        goto release_snapshot;
    if (HasOption(argc, argv, "--sky-only")) {
        snapshot.world.instanceCount = 0;
        snapshot.world.mirrorActive = 0;
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
    if (!HasOption(argc, argv, "--sky-only") &&
        !ModernNativeGpuHasDraws()) {
        fprintf(stderr, "rage-frame-replay: snapshot produced no draws\n");
        goto release_renderer;
    }
    {
        const char *probe = OptionValue(argc, argv, "--probe");
        if (probe != NULL) {
            char *end;
            long probeX = strtol(probe, &end, 10);
            long probeY = *end == ',' ? strtol(end + 1, &end, 10) : -1;
            if (*end != '\0' || probeX < 0 || probeY < 0 ||
                probeX >= width || probeY >= height ||
                !ModernNativeGpuWriteProbe(stdout, (int)probeX, (int)probeY,
                                           width, height)) {
                fprintf(stderr,
                        "rage-frame-replay: --probe expects in-bounds X,Y\n");
                goto release_renderer;
            }
        }
    }
    command = SDL_AcquireGPUCommandBuffer(device);
    if (command == NULL) {
        fprintf(stderr, "rage-frame-replay: command buffer: %s\n",
                SDL_GetError());
        goto release_renderer;
    }
    ModernNativeGpuDraw(command, color, depth, 1, height);
    if (!SDL_SubmitGPUCommandBuffer(command)) {
        fprintf(stderr, "rage-frame-replay: submit: %s\n", SDL_GetError());
        goto release_renderer;
    }
    if (!ModernWriteTexturePpm(device, color, width, height, outputPath)) {
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
    RenderWorldSnapshotRelease(&snapshot);
cleanup:
    return result;
}
