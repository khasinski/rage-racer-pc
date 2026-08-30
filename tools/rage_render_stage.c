/*
 * Renders named assets on an empty stage, with no game and no window.
 *
 * rage-frame-replay answers "what did this captured frame look like". This
 * answers "what does this thing look like from here", which is the question
 * you need when the subject is one car, or the join between two pieces of
 * track, and the angle you care about is one no captured frame contains.
 */

#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "port/modern/modern_assets.h"
#include "port/modern/modern_native_gpu.h"
#include "port/modern/modern_texture_dump.h"
#include "port/runtime_config.h"
#include "render/render_stage.h"
#include "render/rmesh.h"

#include <math.h>

/* The legacy archive override provider shares one translation unit with the
 * semantic mod root queried by ModernAssets. A stage never loads the retail
 * archive, so it has no PS1 asset arena to measure. */
size_t PortAssetRoomAt(const void *at) {
    (void)at;
    return 0;
}

#define MAX_POSES 32

static void Usage(const char *program) {
    fprintf(stderr,
            "usage: %s --assets NATIVE_ASSETS --pose SET:KEY:MESH\n"
            "           [--at X,Y,Z] [--rot X,Y,Z] [--variant N] [--quat]\n"
            "           [--pose ... repeated ...]\n"
            "           [--azimuth D] [--elevation D] [--distance F]\n"
            "           [--fov D] [--target X,Y,Z]\n"
            "           [--width W] [--height H] [--output stage.ppm]\n"
            "  SET is model, course, terrain, track1 or track2.\n"
            "  --at, --rot and --variant apply to the preceding --pose.\n"
"  --rot is the scene Euler triple in degrees; Y turns the\n"
"  subject on the spot and X tips it nose over tail.\n"
"  --elevation is positive above the subject.\n"
            "  --quat poses through the quaternion the angles describe, which\n"
            "  is the form the game uses for cars.\n"
            "  --sweep N turns every pose through a full circle in N steps,\n"
            "  writing stage-000.ppm ... alongside --output.\n",
            program);
}

static int ParseSet(const char *text, RageRenderAssetSet *out) {
    if (strcmp(text, "model") == 0) *out = RAGE_RENDER_ASSET_MODEL_BANK;
    else if (strcmp(text, "course") == 0) *out = RAGE_RENDER_ASSET_COURSE;
    else if (strcmp(text, "terrain") == 0) *out = RAGE_RENDER_ASSET_TERRAIN;
    else if (strcmp(text, "track1") == 0)
        *out = RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1;
    else if (strcmp(text, "track2") == 0)
        *out = RAGE_RENDER_ASSET_TRACK_MODEL_BANK_2;
    else return 0;
    return 1;
}

/* SET:KEY[:MESH] */
static int ParsePose(const char *text, RageRenderPose *pose) {
    char buffer[64];
    char *cursor;
    char *field;
    size_t length = strlen(text);
    if (length >= sizeof(buffer)) return 0;
    memcpy(buffer, text, length + 1);
    cursor = buffer;
    field = cursor;
    cursor = strchr(cursor, ':');
    if (cursor == NULL) return 0;
    *cursor++ = '\0';
    RenderPoseDefaults(pose);
    if (!ParseSet(field, &pose->assetSet)) return 0;
    field = cursor;
    cursor = strchr(cursor, ':');
    if (cursor != NULL) *cursor++ = '\0';
    pose->assetKey = (uint32_t)strtoul(field, NULL, 10);
    if (cursor != NULL) pose->mesh = (uint32_t)strtoul(cursor, NULL, 10);
    return 1;
}

/*
 * Where the posed meshes actually are. Imported geometry keeps the
 * coordinates it was authored in, so a track piece can sit tens of thousands
 * of units from the origin. Framing the camera on the subject's own bounds is
 * what lets a caller name a thing and see it, rather than having to know
 * where in the world it was built.
 */
static void RotateEuler(RageRenderVec3 *v, const RageRenderVec3 *degrees) {
    const float toRadians = 3.14159265358979323846f / 180.0f;
    float x = degrees->x * toRadians;
    float y = degrees->y * toRadians;
    float z = degrees->z * toRadians;
    float ty, tz, tx;
    /* The scene's local-to-world order, X then Y then Z. */
    ty = v->y * cosf(x) - v->z * sinf(x);
    tz = v->y * sinf(x) + v->z * cosf(x);
    v->y = ty; v->z = tz;
    tx = v->x * cosf(y) + v->z * sinf(y);
    tz = -v->x * sinf(y) + v->z * cosf(y);
    v->x = tx; v->z = tz;
    tx = v->x * cosf(z) - v->y * sinf(z);
    ty = v->x * sinf(z) + v->y * cosf(z);
    v->x = tx; v->y = ty;
}

static int StageBounds(const RageRenderWorld *world, RageRenderVec3 *center,
                       float *radius) {
    uint32_t index;
    int found = 0;
    for (index = 0; index < world->instanceCount; index++) {
        const RageRenderMeshInstance *instance = &world->instances[index];
        const RageRuntimeCachedMesh *cached = ModernAssetsFind(instance);
        RageRenderVec3 point;
        float local[3];
        float localRadius;
        if (cached == NULL ||
            !RuntimeMeshBounds(&cached->mesh, instance->mesh, local,
                               &localRadius))
            continue;
        point.x = local[0];
        point.y = local[1];
        point.z = local[2];
        RotateEuler(&point, &instance->transform.rotation);
        point.x += instance->transform.position.x;
        point.y += instance->transform.position.y;
        point.z += instance->transform.position.z;
        if (!found) {
            *center = point;
            *radius = localRadius;
            found = 1;
            continue;
        }
        {
            /* Grow the sphere to take in the new one. */
            float dx = point.x - center->x;
            float dy = point.y - center->y;
            float dz = point.z - center->z;
            float distance = sqrtf(dx * dx + dy * dy + dz * dz);
            float combined;
            if (distance + localRadius <= *radius) continue;
            combined = (distance + localRadius + *radius) * 0.5f;
            if (distance > 0.0f) {
                float step = (combined - *radius) / distance;
                center->x += dx * step;
                center->y += dy * step;
                center->z += dz * step;
            }
            *radius = combined;
        }
    }
    return found;
}

static int ParseTriple(const char *text, RageRenderVec3 *out) {
    return sscanf(text, "%f,%f,%f", &out->x, &out->y, &out->z) == 3;
}

int main(int argc, char **argv) {
    RageRenderStage stage;
    RageRenderPose poses[MAX_POSES];
    RageRenderMeshInstance instances[MAX_POSES];
    RageRenderWorld world;
    const char *assetsPath = NULL;
    const char *outputPath = "stage.ppm";
    const char *drawPath = NULL;
    SDL_GPUDevice *device = NULL;
    SDL_GPUTexture *color = NULL;
    SDL_GPUTexture *depth = NULL;
    SDL_GPUCommandBuffer *command;
    int poseCount = 0;
    int haveTarget = 0;
    int haveDistance = 0;
    int sweep = 0;
    int step;
    int width = 640;
    int height = 480;
    int status = EXIT_FAILURE;
    int index;

    RenderStageDefaults(&stage);
    for (index = 1; index < argc; index++) {
        const char *option = argv[index];
        const char *value = index + 1 < argc ? argv[index + 1] : NULL;
        int wantsValue = 1;
        if (strcmp(option, "--pose") == 0) {
            if (value == NULL || poseCount == MAX_POSES ||
                !ParsePose(value, &poses[poseCount])) {
                fprintf(stderr, "rage-render-stage: bad --pose\n");
                return EXIT_FAILURE;
            }
            poseCount++;
        } else if (strcmp(option, "--quat") == 0) {
            if (poseCount == 0) {
                fprintf(stderr,
                        "rage-render-stage: --quat needs a --pose first\n");
                return EXIT_FAILURE;
            }
            poses[poseCount - 1].useQuaternion = 1;
            index--; /* takes no value */
        } else if (strcmp(option, "--at") == 0 ||
                   strcmp(option, "--rot") == 0 ||
                   strcmp(option, "--variant") == 0) {
            RageRenderPose *pose;
            if (poseCount == 0 || value == NULL) {
                fprintf(stderr, "rage-render-stage: %s needs a --pose first\n",
                        option);
                return EXIT_FAILURE;
            }
            pose = &poses[poseCount - 1];
            if (strcmp(option, "--variant") == 0)
                pose->materialVariant = (uint8_t)strtoul(value, NULL, 10);
            else if (!ParseTriple(value, strcmp(option, "--at") == 0
                                             ? &pose->position
                                             : &pose->rotationDegrees)) {
                fprintf(stderr, "rage-render-stage: %s expects X,Y,Z\n",
                        option);
                return EXIT_FAILURE;
            }
        } else if (strcmp(option, "--assets") == 0) {
            assetsPath = value;
        } else if (strcmp(option, "--output") == 0) {
            outputPath = value;
        } else if (strcmp(option, "--draws") == 0) {
            drawPath = value;
        } else if (strcmp(option, "--azimuth") == 0) {
            stage.azimuthDegrees = (float)atof(value ? value : "0");
        } else if (strcmp(option, "--elevation") == 0) {
            stage.elevationDegrees = (float)atof(value ? value : "0");
        } else if (strcmp(option, "--distance") == 0) {
            stage.distance = (float)atof(value ? value : "0");
            haveDistance = 1;
        } else if (strcmp(option, "--fov") == 0) {
            stage.verticalFovDegrees = (float)atof(value ? value : "0");
        } else if (strcmp(option, "--target") == 0) {
            if (value == NULL || !ParseTriple(value, &stage.target)) {
                fprintf(stderr, "rage-render-stage: --target expects X,Y,Z\n");
                return EXIT_FAILURE;
            }
            haveTarget = 1;
        } else if (strcmp(option, "--sweep") == 0) {
            sweep = atoi(value ? value : "0");
            if (sweep < 1 || sweep > 360) {
                fprintf(stderr, "rage-render-stage: --sweep expects 1..360\n");
                return EXIT_FAILURE;
            }
        } else if (strcmp(option, "--width") == 0) {
            width = atoi(value ? value : "0");
        } else if (strcmp(option, "--height") == 0) {
            height = atoi(value ? value : "0");
        } else {
            Usage(argv[0]);
            return EXIT_FAILURE;
        }
        if (wantsValue) index++;
    }

    if (assetsPath == NULL || poseCount == 0 || width < 16 || height < 16) {
        Usage(argv[0]);
        return EXIT_FAILURE;
    }
    if (!RenderStageCompose(&world, instances, MAX_POSES, &stage, poses,
                            (uint32_t)poseCount)) {
        fprintf(stderr, "rage-render-stage: nothing to place\n");
        return EXIT_FAILURE;
    }
    if (!RuntimeConfigInit(argc, argv)) return EXIT_FAILURE;
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "rage-render-stage: SDL_Init: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, true, NULL);
    if (device == NULL) {
        fprintf(stderr, "rage-render-stage: GPU: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    if (!ModernAssetsInitRoot(assetsPath) || !ModernNativeGpuInit(device)) {
        fprintf(stderr, "rage-render-stage: cannot open %s\n", assetsPath);
        goto release_gpu;
    }
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
        fprintf(stderr, "rage-render-stage: target: %s\n", SDL_GetError());
        goto release_renderer;
    }
    ModernAssetsWarmWorld(&world);
    {
        RageRenderVec3 center;
        float radius = 0.0f;
        if ((!haveTarget || !haveDistance) && StageBounds(&world, &center,
                                                          &radius)) {
            const float toRadians = 3.14159265358979323846f / 180.0f;
            if (!haveTarget) stage.target = center;
            if (!haveDistance) {
                float half = stage.verticalFovDegrees * 0.5f * toRadians;
                float fit = sinf(half);
                /* A margin so the subject does not touch the frame edge. */
                stage.distance = fit > 0.0f ? radius / fit * 1.25f
                                            : radius * 4.0f;
            }
            if (stage.farPlane < stage.distance + radius * 4.0f)
                stage.farPlane = stage.distance + radius * 4.0f;
            RenderStageCamera(&stage, &world.camera);
            world.previousCamera = world.camera;
            fprintf(stderr,
                    "rage-render-stage: framing centre=%.0f,%.0f,%.0f "
                    "radius=%.0f distance=%.0f\n",
                    center.x, center.y, center.z, radius, stage.distance);
        }
    }
    ModernNativeGpuPrepare(&world, (float)width / (float)height);
    if (!ModernNativeGpuHasDraws()) {
        fprintf(stderr, "rage-render-stage: the stage produced no draws\n");
        goto release_renderer;
    }
    /* One turn of the subject, or one still. Holding the device open across
     * the whole sweep is what makes an angle sweep affordable as a test:
     * standing the GPU up costs far more than any one frame. */
    for (step = 0; step < (sweep > 0 ? sweep : 1); step++) {
        char sweepPath[1024];
        const char *framePath = outputPath;
        if (sweep > 0) {
            RageRenderPose turned[MAX_POSES];
            int pose;
            float turn = (float)step * 360.0f / (float)sweep;
            size_t stem = strlen(outputPath);
            while (stem > 0 && outputPath[stem - 1] != '.') stem--;
            if (stem == 0) stem = strlen(outputPath) + 1;
            if (snprintf(sweepPath, sizeof(sweepPath), "%.*s-%03d.ppm",
                         (int)(stem - 1), outputPath, step) >=
                (int)sizeof(sweepPath)) {
                fprintf(stderr, "rage-render-stage: output path too long\n");
                goto release_renderer;
            }
            framePath = sweepPath;
            /* Recompose rather than nudge the instances, so a pose keeps
             * whichever rotation form it was asked for. */
            for (pose = 0; pose < poseCount; pose++) {
                turned[pose] = poses[pose];
                turned[pose].rotationDegrees.y += turn;
            }
            RenderStageCompose(&world, instances, MAX_POSES, &stage, turned,
                               (uint32_t)poseCount);
            /* The renderer keeps its built geometry until the frame number
             * moves, so each step of the sweep is its own frame. */
            world.frame = (uint64_t)step + 2;
            ModernNativeGpuPrepare(&world, (float)width / (float)height);
            if (!ModernNativeGpuHasDraws()) {
                fprintf(stderr,
                        "rage-render-stage: no draws at %.0f degrees\n", turn);
                goto release_renderer;
            }
        }
        command = SDL_AcquireGPUCommandBuffer(device);
        if (command == NULL) {
            fprintf(stderr, "rage-render-stage: command buffer: %s\n",
                    SDL_GetError());
            goto release_renderer;
        }
        ModernNativeGpuDraw(command, color, depth, 1);
        if (!SDL_SubmitGPUCommandBuffer(command)) {
            fprintf(stderr, "rage-render-stage: submit: %s\n", SDL_GetError());
            goto release_renderer;
        }
        if (!ModernWriteTexturePpm(device, color, width, height, framePath)) {
            fprintf(stderr, "rage-render-stage: cannot write %s\n", framePath);
            goto release_renderer;
        }
    }
    if (drawPath != NULL) {
        FILE *file = fopen(drawPath, "w");
        if (file == NULL || !ModernNativeGpuWriteDrawDump(file)) {
            if (file != NULL) fclose(file);
            fprintf(stderr, "rage-render-stage: cannot write %s\n", drawPath);
            goto release_renderer;
        }
        fclose(file);
    }
    fprintf(stderr, "rage-render-stage: poses=%d output=%s\n", poseCount,
            outputPath);
    status = EXIT_SUCCESS;

release_renderer:
    if (color != NULL) SDL_ReleaseGPUTexture(device, color);
    if (depth != NULL) SDL_ReleaseGPUTexture(device, depth);
    ModernNativeGpuShutdown();
release_gpu:
    SDL_DestroyGPUDevice(device);
    return status;
}
