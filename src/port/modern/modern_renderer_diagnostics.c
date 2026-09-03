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
#include "render/render_world_snapshot.h"

extern int32_t g_TrackTexturePageWanted;
extern int32_t g_TrackTextureCursorRow;
extern int32_t g_TrackTextureTargetRow;
extern int32_t g_IsEnvironmentMode4;

static int WriteModern(const RageModernDiagnosticFrame *frame,
                       const char *path) {
    return frame != NULL && path != NULL && frame->texture != NULL &&
           ModernWriteTexturePpm(frame->device, frame->texture,
                                     frame->width, frame->height, path);
}

void ModernDiagnosticsMaybeDump(
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
    if (snapshot == NULL || output == NULL) return;
    if (!initialized) {
        const char *frameText = RuntimeConfigGet("diagnostics.modern_dump_frame");
        const char *everyText = RuntimeConfigGet("diagnostics.modern_dump_every");
        const char *sceneText = RuntimeConfigGet(
            "diagnostics.modern_dump_scene_id");
        const char *timerText = RuntimeConfigGet(
            "diagnostics.modern_dump_timer");
        path = RuntimeConfigGet("diagnostics.modern_dump");
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
    if (RuntimeConfigEnabled("diagnostics.modern_dump_scene")) {
        char scenePath[512];
        const RageRenderWorld *world = ModernNativeGpuPreparedWorld();
        FILE *file;
        snprintf(scenePath, sizeof(scenePath), "%s.scene.bin", path);
        file = fopen(scenePath, "wb");
        if (file != NULL) {
            fwrite(snapshot, sizeof(*snapshot), 1, file);
            fclose(file);
        }
        if (world != NULL) {
            snprintf(scenePath, sizeof(scenePath), "%s.world.bin", path);
            if (!RenderWorldSnapshotWrite(scenePath, world))
                fprintf(stderr,
                        "rage-port: render-world dump failed: %s\n",
                        scenePath);
            snprintf(scenePath, sizeof(scenePath), "%s.draws.txt", path);
            file = fopen(scenePath, "w");
            if (file != NULL) {
                if (!ModernNativeGpuWriteDrawDump(file))
                    fprintf(stderr, "rage-port: draw dump failed: %s\n",
                            scenePath);
                fclose(file);
            }
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

/* The HUD is built from sprite sheets and fonts that live in VRAM, so a
 * marker that does not carry VRAM cannot be used to draw one offline. */
static void WriteVram(const char *path) {
    int width = 0, height = 0;
    unsigned short *pixels = Psyz_VideoAllocCapturedVram(&width, &height);
    FILE *file;
    if (pixels == NULL) return;
    file = fopen(path, "wb");
    if (file != NULL) {
        fwrite(pixels, sizeof(*pixels), (size_t)width * height, file);
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
        if (batch->cellCount > 0) {
            fprintf(file,
                    "terrain[%d] mirror=%d cells=%d shift=%d "
                    "cell0=%d,%d,%d#%d\n",
                    index, batch->mirror, batch->cellCount, batch->otShift,
                    batch->cells[0][0], batch->cells[0][1],
                    batch->cells[0][2], batch->cells[0][3]);
        } else {
            fprintf(file,
                    "terrain[%d] mirror=%d cells=0 shift=%d cell0=none\n",
                    index, batch->mirror, batch->otShift);
        }
    }
    {
        const RageRenderWorld *world = GameRenderWorldCurrent();
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

void ModernDiagnosticsCheckMarker(
    const RageSceneSnapshot *snapshot,
    const RageModernDiagnosticFrame *output,
    int haveModernImage) {
    static int wasDown;
    static int burstLeft;
    static int markerIndex = -1;
    const bool *keys;
    int down;
    int pressed;
    if (snapshot == NULL || output == NULL) return;
    keys = SDL_GetKeyboardState(NULL);
    down = keys != NULL && keys[SDL_SCANCODE_M];
    pressed = down && !wasDown;
    /* A marker that only a key can take needs somebody at the keyboard, and
     * the offline tools that read markers then cannot be run from a script.
     * Naming a frame takes the same capture without anyone present. */
    {
        static long atFrame = -2;
        static long every;
        static long nextFrame;
        if (atFrame == -2) {
            const char *value = RuntimeConfigGet("diagnostics.marker_frame");
            const char *repeat = RuntimeConfigGet("diagnostics.marker_every");
            atFrame = value != NULL ? strtol(value, NULL, 0) : -1;
            every = repeat != NULL ? strtol(repeat, NULL, 0) : 0;
            nextFrame = atFrame;
        }
        if (nextFrame >= 0 && (long)snapshot->frameCounter >= nextFrame) {
            /* Repeating matters for anything whose correctness depends on the
             * camera moving: one marker cannot tell a horizon that follows
             * the pitch from one that follows half of it. */
            nextFrame = every > 0
                ? (long)snapshot->frameCounter + every : -1;
            pressed = 1;
        }
    }
    char path[256];
    FILE *file;
    int index;
    wasDown = down;
    if (pressed) burstLeft = 4;
    if (pressed && output->ringTextures != NULL) {
        PlatformEnsureDirectory("markers");
        for (index = 0; index < output->ringCount; index++) {
            int slot = (output->ringNext + index) % output->ringCount;
            if (output->ringFrames[slot] == 0) continue;
            snprintf(path, sizeof(path), "markers/ring-%02d-f%u-%s.ppm",
                     index, output->ringFrames[slot],
                     output->ringInterpolation[slot] < -1.5f ? "lerp" : "snap");
            ModernWriteTexturePpm(output->device,
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
    PlatformEnsureDirectory("markers");
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
    snprintf(path, sizeof(path), "markers/marker-%d-vram.raw", index);
    WriteVram(path);
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
            if (!RenderWorldSnapshotWrite(path, world))
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
    {
        unsigned char palette[9][3];
        int slot;
        snprintf(path, sizeof(path), "markers/marker-%d-palette.txt", index);
        file = fopen(path, "w");
        if (file != NULL) {
            GameRenderWorldEnvironmentPalette(palette);
            for (slot = 0; slot < 9; slot++)
                fprintf(file, "slot %d = %d,%d,%d\n", slot, palette[slot][0],
                        palette[slot][1], palette[slot][2]);
            fclose(file);
        }
    }
    fprintf(stderr, "rage-port: marker %d saved (frame=%u scene=%d timer=%d)\n",
            index, snapshot->frameCounter, snapshot->sceneId,
            snapshot->sceneTimer);
}
