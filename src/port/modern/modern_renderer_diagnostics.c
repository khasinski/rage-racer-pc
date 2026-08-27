#include "modern_renderer_diagnostics.h"

#include <SDL3/SDL.h>
#include <psyz/video.h>
#include <stdio.h>
#include <stdlib.h>
#include "modern_texture_dump.h"
#include "modern_native_gpu.h"
#include "../runtime_config.h"
#include "../platform_paths.h"
#include "../include/rage/render_world_game.h"
#include "render/render_world.h"
#include "render/render_world_snapshot.h"

extern int32_t g_TrackTexturePageWanted;
extern int32_t g_TrackTextureCursorRow;
extern int32_t g_TrackTextureTargetRow;
extern int32_t g_IsEnvironmentMode4;

static int WriteModern(const RageModernDiagnosticFrame *frame,
                       const char *path) {
    return frame->texture != NULL &&
           RageModernWriteTexturePpm(frame->device, frame->texture,
                                     frame->width, frame->height, path);
}

void RageModernDiagnosticsMaybeDump(
    const RageSceneSnapshot *snapshot,
    const RageModernDiagnosticFrame *output) {
    static int initialized;
    static const char *path;
    static long frame = -1;
    static long scene = -1;
    static long timer = -1;
    static long every;
    static long lastDumped = -1;
    static int done;
    if (!initialized) {
        const char *frameText = RageRuntimeConfigGetLegacy(
            "diagnostics.modern_dump_frame", "RAGE_PORT_MODERN_DUMP_FRAME");
        const char *everyText = RageRuntimeConfigGetLegacy(
            "diagnostics.modern_dump_every", "RAGE_PORT_MODERN_DUMP_EVERY");
        const char *sceneText = RageRuntimeConfigGet(
            "diagnostics.modern_dump_scene_id");
        const char *timerText = RageRuntimeConfigGet(
            "diagnostics.modern_dump_timer");
        path = RageRuntimeConfigGetLegacy(
            "diagnostics.modern_dump", "RAGE_PORT_MODERN_DUMP");
        if (frameText != NULL) frame = strtol(frameText, NULL, 0);
        if (everyText != NULL) every = strtol(everyText, NULL, 0);
        if (sceneText != NULL) scene = strtol(sceneText, NULL, 0);
        if (timerText != NULL) timer = strtol(timerText, NULL, 0);
        initialized = 1;
    }
    if (path == NULL || done ||
        (frame >= 0 && (long)snapshot->frameCounter < frame) ||
        (scene >= 0 && snapshot->sceneId != scene) ||
        (timer >= 0 && (long)snapshot->sceneTimer < timer)) return;
    if (every > 0) {
        char numbered[512];
        if (lastDumped >= 0 &&
            (long)snapshot->frameCounter < lastDumped + every) return;
        snprintf(numbered, sizeof(numbered), "%s-%06u.ppm", path,
                 snapshot->frameCounter);
        if (WriteModern(output, numbered))
            lastDumped = (long)snapshot->frameCounter;
        return;
    }
    if (WriteModern(output, path))
        fprintf(stderr, "rage-port: modern dump frame=%u -> %s\n",
                snapshot->frameCounter, path);
    if (RageRuntimeConfigEnabled("diagnostics.modern_dump_scene",
                                 "RAGE_PORT_MODERN_DUMP_SCENE")) {
        char scenePath[512];
        FILE *file;
        snprintf(scenePath, sizeof(scenePath), "%s.scene.bin", path);
        file = fopen(scenePath, "wb");
        if (file != NULL) {
            fwrite(snapshot, sizeof(*snapshot), 1, file);
            fclose(file);
        }
    }
    done = 1;
}

static void WriteCompat(const char *path) {
    int width = 0, height = 0;
    unsigned char *pixels = Psyz_VideoAllocCapturedFrame(&width, &height);
    FILE *file;
    if (pixels == NULL) return;
    file = fopen(path, "wb");
    if (file != NULL) {
        fprintf(file, "P6\n%d %d\n255\n", width, height);
        fwrite(pixels, 3, (size_t)width * height, file);
        fclose(file);
    }
    free(pixels);
}

static void WriteSceneInfo(FILE *file, const RageSceneSnapshot *snapshot,
                           const RageModernDiagnosticFrame *output,
                           int haveModernImage) {
    int index;
    fprintf(file,
            "frame=%u scene=%d timer=%d displayHeight=%d\n"
            "modernImage=%d target=%dx%d logicalW=%.1f fps=%d\n"
            "camera pos=%d,%d,%d\n"
            "view=[%d %d %d / %d %d %d / %d %d %d] t=%d,%d,%d\n"
            "trackTextures wanted=%d cursor=%d target=%d envMode4=%d\n"
            "draws=%d terrain=%d faces=%d packets=%d\n",
            snapshot->frameCounter, snapshot->sceneId, snapshot->sceneTimer,
            snapshot->displayHeight, haveModernImage, output->width,
            output->height, output->logicalWidth, output->fps,
            snapshot->viewPosition[0], snapshot->viewPosition[1],
            snapshot->viewPosition[2], snapshot->viewMatrix.m[0][0],
            snapshot->viewMatrix.m[0][1], snapshot->viewMatrix.m[0][2],
            snapshot->viewMatrix.m[1][0], snapshot->viewMatrix.m[1][1],
            snapshot->viewMatrix.m[1][2], snapshot->viewMatrix.m[2][0],
            snapshot->viewMatrix.m[2][1], snapshot->viewMatrix.m[2][2],
            snapshot->viewMatrix.t[0], snapshot->viewMatrix.t[1],
            snapshot->viewMatrix.t[2], g_TrackTexturePageWanted,
            g_TrackTextureCursorRow, g_TrackTextureTargetRow,
            g_IsEnvironmentMode4, snapshot->drawCount,
            snapshot->terrainCount, snapshot->faceCount,
            snapshot->packetCount);
    for (index = 0; index < snapshot->drawCount && index < 64; index++) {
        const RageCaptureModelDraw *draw = &snapshot->draws[index];
        fprintf(file,
                "draw[%d] kind=%d model=%d mirror=%d table=%d "
                "otBase=%d shift=%d trans=%d,%d,%d\n",
                index, draw->kind, draw->modelIndex, draw->mirror, draw->table,
                draw->otBaseBias, draw->otShift, draw->gte.rot.t[0],
                draw->gte.rot.t[1], draw->gte.rot.t[2]);
    }
    for (index = 0; index < snapshot->terrainCount; index++) {
        const RageCaptureTerrainBatch *batch = &snapshot->terrain[index];
        fprintf(file,
                "terrain[%d] mirror=%d cells=%d shift=%d "
                "cell0=%d,%d,%d#%d\n",
                index, batch->mirror, batch->cellCount, batch->otShift,
                batch->cells[0][0], batch->cells[0][1], batch->cells[0][2],
                batch->cells[0][3]);
    }
    {
        const RageRenderWorld *world = RageGameRenderWorldCurrent();
        uint32_t dynamicCount = 0;
        if (world != NULL) {
            for (uint32_t worldIndex = 0;
                 worldIndex < world->instanceCount; worldIndex++) {
                const RageRenderMeshInstance *instance =
                    &world->instances[worldIndex];
                if (instance->assetSet != RAGE_RENDER_ASSET_COURSE ||
                    instance->entity < 0x30000u ||
                    instance->entity >= 0x40000u) continue;
                dynamicCount++;
                fprintf(file,
                        "nativeDynamic[%u] entity=%x mesh=%u variant=%u "
                        "pass=%d flags=%x pos=%.1f,%.1f,%.1f\n",
                        dynamicCount - 1, instance->entity, instance->mesh,
                        instance->materialVariant, instance->pass,
                        instance->flags, instance->transform.position.x,
                        instance->transform.position.y,
                        instance->transform.position.z);
            }
            fprintf(file, "nativeWorld frame=%llu instances=%u dynamic=%u\n",
                    (unsigned long long)world->frame, world->instanceCount,
                    dynamicCount);
        } else {
            fprintf(file, "nativeWorld unavailable\n");
        }
    }
}

void RageModernDiagnosticsCheckMarker(
    const RageSceneSnapshot *snapshot,
    const RageModernDiagnosticFrame *output,
    int haveModernImage) {
    static int wasDown;
    static int burstLeft;
    static int markerIndex = -1;
    const bool *keys = SDL_GetKeyboardState(NULL);
    int down = keys != NULL && keys[SDL_SCANCODE_M];
    int pressed = down && !wasDown;
    char path[256];
    FILE *file;
    int index;
    wasDown = down;
    if (pressed) burstLeft = 4;
    if (pressed && output->ringTextures != NULL) {
        RagePlatformEnsureDirectory("markers");
        for (index = 0; index < output->ringCount; index++) {
            int slot = (output->ringNext + index) % output->ringCount;
            if (output->ringFrames[slot] == 0) continue;
            snprintf(path, sizeof(path), "markers/ring-%02d-f%u-%s.ppm",
                     index, output->ringFrames[slot],
                     output->ringInterpolation[slot] < -1.5f ? "lerp" : "snap");
            RageModernWriteTexturePpm(output->device,
                                      output->ringTextures[slot], output->width,
                                      output->height, path);
            if (output->ringScenes != NULL) {
                snprintf(path, sizeof(path),
                         "markers/ring-%02d-f%u-scene.bin", index,
                         output->ringFrames[slot]);
                file = fopen(path, "wb");
                if (file != NULL) {
                    fwrite(&output->ringScenes[slot], sizeof(*snapshot), 1, file);
                    fclose(file);
                }
            }
        }
        fprintf(stderr, "rage-port: ring of %d frames dumped\n",
                output->ringCount);
    }
    if (burstLeft <= 0) return;
    burstLeft--;
    RagePlatformEnsureDirectory("markers");
    if (markerIndex < 0) {
        markerIndex = 0;
        for (index = 0; index < 1000; index++) {
            snprintf(path, sizeof(path), "markers/marker-%d-info.txt", index);
            file = fopen(path, "r");
            if (file != NULL) {
                fclose(file);
                markerIndex = index + 1;
            }
        }
    }
    index = markerIndex++;
    if (haveModernImage) {
        snprintf(path, sizeof(path), "markers/marker-%d-modern.ppm", index);
        WriteModern(output, path);
    }
    snprintf(path, sizeof(path), "markers/marker-%d-compat.ppm", index);
    WriteCompat(path);
    snprintf(path, sizeof(path), "markers/marker-%d-scene.bin", index);
    file = fopen(path, "wb");
    if (file != NULL) {
        fwrite(snapshot, sizeof(*snapshot), 1, file);
        fclose(file);
    }
    {
        const RageRenderWorld *world = ModernNativeGpuPreparedWorld();
        if (world != NULL) {
            snprintf(path, sizeof(path), "markers/marker-%d-world.bin", index);
            if (!RageRenderWorldSnapshotWrite(path, world))
                fprintf(stderr,
                        "rage-port: marker %d render-world save failed\n",
                        index);
            snprintf(path, sizeof(path), "markers/marker-%d-draws.txt", index);
            file = fopen(path, "w");
            if (file != NULL) {
                if (!ModernNativeGpuWriteDrawDump(file))
                    fprintf(stderr,
                            "rage-port: marker %d draw dump failed\n", index);
                fclose(file);
            }
        }
    }
    snprintf(path, sizeof(path), "markers/marker-%d-info.txt", index);
    file = fopen(path, "w");
    if (file != NULL) {
        WriteSceneInfo(file, snapshot, output, haveModernImage);
        fclose(file);
    }
    fprintf(stderr, "rage-port: marker %d saved (frame=%u scene=%d timer=%d)\n",
            index, snapshot->frameCounter, snapshot->sceneId,
            snapshot->sceneTimer);
}
