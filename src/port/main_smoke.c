#include <psyz.h>
#include <psyz/video.h>
#include <psyz/audio.h>
#include <psyz/cd.h>
#include <psyz/spu.h>
#include <libetc.h>

#include <stdio.h>
#include <stdlib.h>

#include "host_storage.h"

#include "input_config.h"
#include "content_options.h"
#include "diagnostic_log.h"
#include "port_config.h"
#include "runtime_config.h"
#include "timing_control.h"
#include "modern/modern_renderer.h"
#include "modern/scene_capture.h"
#include "game/player_car_internal.h"
#include "game/input_internal.h"
#include "game/state.h"
#include "game/race.h"
#include "game/memcard.h"
#include "game/audio.h"
#include "game/asset.h"
#include "game/car_internal.h"
#include "game/scratchpad.h"
#include "game/render_internal.h"
#include "game/sound.h"

void MainLoop(void);
int RageMapPs1Scratchpad(void);
int RageInitNativeGameData(void);
int RageHostInitDisc(void);
extern int g_SceneId;
extern int g_FrameCounter;
extern int g_FrontendState;
extern int GameMenuBusy;
extern int g_MenuScreen;
extern int g_MenuViewAngle;
extern int g_MenuViewAngleTarget;
extern int g_MenuViewOffset;
extern int g_MenuViewSpin;
extern int g_FadeLevel;
extern int g_FrameSyncThreshold;
extern s32 g_SkyRowBase;
extern s32 g_MirrorPanelY;
extern Vec4 g_MirrorVisibleCellList[];
extern u32 g_MainVisibleCellMask[];
extern u32 g_MirrorVisibleCellMask[];
extern unsigned long long g_RageGt4FacesEmitted;
extern unsigned g_RageGt4ColorMaximum;
extern int g_RageGt4DepthMinimum;
extern int g_RageGt4DepthMaximum;
extern unsigned long long g_RageGt4ClipPositive;
extern unsigned long long g_RageGt4ClipNegative;
extern unsigned long long g_RageGt4RejectOffscreen;
extern unsigned long long g_RageGt4RejectBackface;
extern unsigned long long g_RageGt4RejectDepth;
extern unsigned long long g_RageModelRejectBackface;
extern unsigned long long g_RageTerrainSecondTriangleVisible;
extern unsigned long long g_RageTerrainChildRejectBackface;
extern unsigned long long g_RageTerrainChildSecondTriangleVisible;
int RageRetireCameraActive(void);

int RageWriteCapturedFrame(const char *path) {
    unsigned char *pixels;
    FILE *output;
    int width;
    int height;
    unsigned short *vram = NULL;
    int vramWidth = 0;
    int vramHeight = 0;
    PsyzVideoCaptureState captureState;

    if (path == NULL || path[0] == '\0') return 1;
    /* DrawOTag is asynchronous. The synchronized smoke capture runs directly
     * after submission, so execute the queued OT before reading either its
     * page or full VRAM; otherwise the PPM describes the previous buffer while
     * the manifest describes the current game state. */
    DrawSync(0);
    /* Prime the reusable GPU download buffer before the asserted capture. */
    if (RageRuntimeConfigEnabled("capture.draw_page", "RAGE_PORT_CAPTURE_DRAW_PAGE")) {
        pixels = Psyz_VideoAllocCapturedDrawPage(&width, &height);
    } else {
        pixels = Psyz_VideoAllocCapturedFrame(&width, &height);
    }
    if (pixels == NULL) return 0;
    free(pixels);
    if (RageRuntimeConfigEnabled("capture.draw_page", "RAGE_PORT_CAPTURE_DRAW_PAGE")) {
        pixels = Psyz_VideoAllocCapturedDrawPage(&width, &height);
    } else {
        pixels = Psyz_VideoAllocCapturedFrame(&width, &height);
    }
    if (pixels == NULL) return 0;
    if (RageRuntimeConfigEnabled("capture.vram_sidecar", "RAGE_PORT_CAPTURE_VRAM_SIDECAR")) {
        size_t pathLength = strlen(path);
        char *vramPath;
        FILE *vramOutput;
        vram = Psyz_VideoAllocCapturedVram(&vramWidth, &vramHeight);
        if (vram == NULL || vramWidth != 1024 || vramHeight != 512) {
            free(vram);
            free(pixels);
            return 0;
        }
        vramPath = malloc(pathLength + 6);
        if (vramPath == NULL) {
            free(vram);
            free(pixels);
            return 0;
        }
        memcpy(vramPath, path, pathLength + 1);
        if (pathLength >= 4 && strcmp(vramPath + pathLength - 4, ".ppm") == 0)
            memcpy(vramPath + pathLength - 4, ".vram", 6);
        else
            memcpy(vramPath + pathLength, ".vram", 6);
        vramOutput = fopen(vramPath, "wb");
        free(vramPath);
        if (vramOutput == NULL ||
            fwrite(vram, sizeof(*vram), 1024u * 512u, vramOutput) != 1024u * 512u) {
            if (vramOutput != NULL) fclose(vramOutput);
            free(vram);
            free(pixels);
            return 0;
        }
        fclose(vramOutput);
        if (Psyz_VideoGetCaptureState(&captureState) == 0) {
            unsigned hash0 = 2166136261u, hash1 = 2166136261u;
            size_t i;
            for (i = 0; i < 320u * 240u; i++) {
                unsigned short values[2] = {
                    vram[(i / 320u) * 1024u + (i % 320u)],
                    vram[(240u + i / 320u) * 1024u + (i % 320u)]};
                int page;
                for (page = 0; page < 2; page++) {
                    unsigned *hash = page ? &hash1 : &hash0;
                    *hash ^= values[page] & 0xff; *hash *= 16777619u;
                    *hash ^= values[page] >> 8; *hash *= 16777619u;
                }
            }
            fprintf(stderr,
                    "capture-vram path=%s draw=%d,%d,%d,%d display=%d,%d,%d,%d "
                    "page0=%08x page1=%08x\n",
                    path, captureState.draw_x, captureState.draw_y,
                    captureState.draw_w, captureState.draw_h,
                    captureState.display_x, captureState.display_y,
                    captureState.display_w, captureState.display_h, hash0, hash1);
        }
        free(vram);
    }
    output = fopen(path, "wb");
    if (output == NULL) {
        free(pixels);
        return 0;
    }
    fprintf(output, "P6\n%d %d\n255\n", width, height);
    if (fwrite(pixels, 3, (size_t)width * (size_t)height, output) !=
        (size_t)width * (size_t)height) {
        fclose(output);
        free(pixels);
        return 0;
    }
    fclose(output);
    free(pixels);
    return 1;
}

int main(int argc, char **argv) {
    RageInputConfig inputConfig;
    RagePortConfig portConfig;
    int inputIndex;
    char logPath[1024];

    if (!RageRuntimeConfigInit(argc, argv)) return EXIT_FAILURE;
    if (RageRuntimeConfigEnabled("diagnostics.test_log", "RAGE_PORT_TEST_LOG") &&
        !RageDiagnosticLogOpen(logPath, sizeof(logPath))) return EXIT_FAILURE;

    Psyz_SetTitle("Rage Racer smoke");
    if (!RageHostInitStorage()) return EXIT_FAILURE;
    setenv("RAGE_PORT_TEST_MODE", "1", 0);
    Psyz_VideoSetAspectMode(PSYZ_ASPECT_SQUARE);
    Psyz_VideoSetVsyncMode(PSYZ_VSYNC_LIMITLESS);
    PadInit(0);
    if (RageRuntimeConfigEnabled("input.disable_host", "RAGE_PORT_DISABLE_HOST_INPUT")) {
        Psyz_SetHostInputEnabled(0);
    }
    RageInputConfigDefaults(&inputConfig);
    RageInputConfigLoad(&inputConfig, "rage-input.cfg");
    RageInputConfigApplyRuntime(&inputConfig);
    for (inputIndex = 0; inputIndex < RAGE_INPUT_BUTTON_COUNT; inputIndex++) {
        Psyz_SetKeyboardKey(inputIndex, inputConfig.keys[inputIndex]);
    }
    if (!RageHostInitDisc()) return EXIT_FAILURE;
    if (RageRuntimeConfigGetLegacy("trace.spu", "RAGE_PORT_SPU_TRACE") != NULL &&
        Psyz_SpuSetKeyOnTracePath(RageRuntimeConfigGetLegacy("trace.spu", "RAGE_PORT_SPU_TRACE")) != 0) {
        fprintf(stderr, "unable to open SPU trace: %s\n",
                RageRuntimeConfigGetLegacy("trace.spu", "RAGE_PORT_SPU_TRACE"));
        return EXIT_FAILURE;
    }
    if (!RageInitNativeGameData()) return EXIT_FAILURE;
    RageContentOptionsApply();
    if (!RageMapPs1Scratchpad()) return EXIT_FAILURE;
    RagePortConfigDefaults(&portConfig);
    if (RageRuntimeConfigEnabled("video.modern", "RAGE_PORT_MODERN"))
        portConfig.renderer = RAGE_RENDERER_MODERN;
    RagePortConfigApplyRuntime(&portConfig);
    if (RageRuntimeConfigEnabled("video.modern", "RAGE_PORT_MODERN"))
        portConfig.renderer = RAGE_RENDERER_MODERN;
    RagePortConfigSetActive(&portConfig);
    RageTimingInit();
    if (!RageModernInit(&portConfig)) return EXIT_FAILURE;
    MainLoop();
    if (RageRuntimeConfigGetLegacy("dump.spu_ram", "RAGE_PORT_DUMP_SPU_RAM") != NULL) {
        FILE *output = fopen(RageRuntimeConfigGetLegacy("dump.spu_ram", "RAGE_PORT_DUMP_SPU_RAM"), "wb");
        if (output != NULL) {
            fwrite(Psyz_SpuGetRam(), 1, 512 * 1024, output);
            fclose(output);
        }
    }
    if (RageRuntimeConfigGetLegacy("dump.vram", "RAGE_PORT_DUMP_VRAM") != NULL) {
        const char *path = RageRuntimeConfigGetLegacy("dump.vram", "RAGE_PORT_DUMP_VRAM");
        RECT rect = {0, 0, 1024, 512};
        u16 *vram = malloc(1024u * 512u * sizeof(*vram));
        FILE *output;
        if (vram == NULL) return EXIT_FAILURE;
        DrawSync(0);
        StoreImage(&rect, (u_long *)vram);
        DrawSync(0);
        output = fopen(path, "wb");
        if (output == NULL ||
            fwrite(vram, sizeof(*vram), 1024u * 512u, output) != 1024u * 512u) {
            if (output != NULL) fclose(output);
            free(vram);
            return EXIT_FAILURE;
        }
        fclose(output);
        free(vram);
    }
    if (RageRuntimeConfigEnabled("report.initial_state", "RAGE_PORT_SMOKE_INITIAL_STATE")) {
        int enabled = 0;
        int car;
        for (car = 0; car < 13; car++)
            enabled += g_TimeAttackCars[car].enabled != 0;
        printf("initial state: time_attack_enabled=%d selected_car=%d\n",
               enabled, g_TimeAttackSave.carIndex);
    }
    if (RageRuntimeConfigEnabled("report.window_size", "RAGE_PORT_SMOKE_WINDOW_SIZE")) {
        PsyzSize size = Psyz_VideoGetDisplaySize();
        printf("window size: %dx%d\n", size.w, size.h);
    }
    if (RageRuntimeConfigEnabled("report.audio_metrics", "RAGE_PORT_SMOKE_AUDIO_METRICS")) {
        /* The SDL callback updates both the metrics and an optional PCM dump.
         * Stop it before sampling either so a final callback cannot make the
         * two observations disagree by one buffer. */
        Psyz_AudioPause();
        Psyz_AudioLock();
        printf("audio metrics: frames=%llu energy=%llu seq_notes=%llu seq_voices=%llu "
               "pitch_updates=%llu cdda=%d cdda_frames=%llu cdda_energy=%llu "
               "cdda_mix_energy=%llu "
               "reverb_in=%llu reverb_out=%llu "
               "reverb_tail=%llu "
               "loaded=%x cue_bank=%d "
               "vab=%d,%d,%d,%d slots=%d,%d,%d,%d,%d,%d scale=%d\n",
               Psyz_AudioRenderedFrames(), Psyz_AudioRenderedEnergy(),
               Psyz_SeqNoteOnCount(),
               Psyz_SeqVoiceStartCount(),
               Psyz_SndPitchUpdateCount(),
               Psyz_CdAudioPlaying(),
               Psyz_CdAudioFramesPulled(),
               Psyz_CdAudioEnergy(),
               Psyz_SpuCdMixEnergy(),
               Psyz_SpuReverbInputEnergy(),
               Psyz_SpuReverbOutputEnergy(),
               Psyz_SpuReverbTailFrames(),
               g_AudioLoadedSlotMask, g_SoundCueBank,
               g_SoundScale.vabIds[0], g_SoundScale.vabIds[1],
               g_SoundScale.vabIds[2], g_SoundScale.vabIds[3],
               g_EngineSoundState.slotActive[0],
               g_EngineSoundState.slotActive[1],
               g_EngineSoundState.slotActive[2],
               g_EngineSoundState.slotActive[3],
               g_EngineSoundState.slotActive[4],
               g_EngineSoundState.slotActive[5], g_SoundScale.scale);
        Psyz_AudioUnlock();
    }
    if (RageRuntimeConfigEnabled("report.camera_state", "RAGE_PORT_SMOKE_CAMERA_STATE")) {
        {
            RECT paletteRect = {752, 224, 16, 1};
            RECT textureRect = {704, 120, 16, 1};
            u16 palette[16] = {0};
            u16 texture[16] = {0};
            int sample;
            DrawSync(0);
            StoreImage(&paletteRect, (u_long *)palette);
            StoreImage(&textureRect, (u_long *)texture);
            DrawSync(0);
            printf("vram: clut=");
            for (sample = 0; sample < 16; sample++)
                printf("%s%04x", sample ? "," : "", palette[sample]);
            printf(" tex=");
            for (sample = 0; sample < 16; sample++)
                printf("%s%04x", sample ? "," : "", texture[sample]);
            putchar('\n');
        }
        printf("camera: pos=(%d,%d,%d) angle=(%d,%d,%d) mirror=%d "
               "sky_row=%d key=%p",
               SCRATCH_VIEW_X, SCRATCH_VIEW_Y, SCRATCH_VIEW_Z,
               SCRATCH_VIEW_ANGLE_X, SCRATCH_VIEW_ANGLE_Y,
               SCRATCH_VIEW_ANGLE_Z, SCRATCH_MIRROR, g_SkyRowBase,
               (void *)g_RaceIntroCameraCursor);
        printf(" mirror_mtx=%d,%d,%d;%d,%d,%d;%d,%d,%d",
               g_MirrorViewMatrix.m[0][0], g_MirrorViewMatrix.m[0][1],
               g_MirrorViewMatrix.m[0][2], g_MirrorViewMatrix.m[1][0],
               g_MirrorViewMatrix.m[1][1], g_MirrorViewMatrix.m[1][2],
               g_MirrorViewMatrix.m[2][0], g_MirrorViewMatrix.m[2][1],
               g_MirrorViewMatrix.m[2][2]);
        printf(" menu=%d busy=%d view=%d/%d offset=%d spin=%d yaw=%d/%d",
               g_MenuScreen, GameMenuBusy, g_MenuViewAngle,
               g_MenuViewAngleTarget, g_MenuViewOffset, g_MenuViewSpin,
               g_PlayerCar.bodyYaw, g_PlayerCar.modelYaw);
        printf(" scene_timer=%d fade=%d sync=%x",
               g_SceneTimer, g_FadeLevel, g_FrameSyncThreshold);
        if (g_RaceIntroCameraCursor != NULL) {
            printf(" mode=%d start=%d duration=%d key_pos=(%d,%d,%d)",
                   g_RaceIntroCameraCursor->mode,
                   g_RaceIntroCameraCursor->startFrame,
                   g_RaceIntroCameraCursor->duration,
                   g_RaceIntroCameraCursor->x.word,
                   g_RaceIntroCameraCursor->y.word,
                   g_RaceIntroCameraCursor->z.word);
        }
        if (g_CarTable != NULL) {
            CarEntry *entry = &g_CarTable[g_PlayerCarIndex];
            printf(" car=%d paint=%d,%d rgb=%04x,%04x",
                   g_PlayerCarIndex, entry->paintColor1, entry->paintColor2,
                   g_BodyColorPrimary[entry->paintColor1],
                   g_BodyColorPrimary[entry->paintColor2]);
        }
        if (g_ModelBanks[0].modelCount > 0 && g_ModelBanks[0].models[0]) {
            const u8 *stream = g_ModelBanks[0].models[0];
            u32 opcode = stream[0] | ((u32)stream[1] << 8) |
                         ((u32)stream[2] << 16) | ((u32)stream[3] << 24);
            printf(" bank0=%d opcode=%08x face=%08x,%08x,%08x",
                   g_ModelBanks[0].modelCount, opcode,
                   *(const u32 *)(stream + 4), *(const u32 *)(stream + 8),
                   *(const u32 *)(stream + 12));
            {
                int block;
                const u8 *cursor = stream;
                printf(" blocks=");
                for (block = 0; block < 8; block++) {
                    u32 header = cursor[0] | ((u32)cursor[1] << 8) |
                                 ((u32)cursor[2] << 16) |
                                 ((u32)cursor[3] << 24);
                    int type = header & 0xffff;
                    int count = header >> 16;
                    printf("%s%d:%d", block ? "," : "", type, count);
                    if (type == 3 && count > 0) {
                        const u8 *uv = cursor + 4 + 16;
                        printf("[uv=%02x%02x/%02x%02x/%02x%02x/%02x%02x "
                               "clut=%02x%02x tp=%02x%02x]",
                               uv[0], uv[1], uv[4], uv[5], uv[8], uv[9],
                               uv[10], uv[11], uv[2], uv[3], uv[6], uv[7]);
                    }
                    if (header == 0 || (unsigned)type >= 4) break;
                    cursor += 4 + count * (const int[]){16,24,24,32}[type];
                }
            }
        }
        printf(" ref_lap=%d time_text=%s gt4=%llu max=%u z=%d..%d clip=%llu/%llu reject=%llu/%llu/%llu",
               g_BestLapThisRace,
               g_TimeTextBuffer,
               g_RageGt4FacesEmitted,
               g_RageGt4ColorMaximum, g_RageGt4DepthMinimum,
               g_RageGt4DepthMaximum, g_RageGt4ClipPositive,
               g_RageGt4ClipNegative, g_RageGt4RejectOffscreen,
               g_RageGt4RejectBackface, g_RageGt4RejectDepth);
        putchar('\n');
    }
    if (RageRuntimeConfigEnabled("capture.visible_cells", "RAGE_PORT_SMOKE_VISIBLE_CELLS")) {
        int cell;
        for (cell = 0; cell < 64; cell++) {
            const Vec4 *mainEntry = &g_VisibleCellList[cell];
            const Vec4 *entry = &g_MirrorVisibleCellList[cell];
            printf("visible-cell %d=%d,%d,%d,%d\n", cell, mainEntry->x,
                   mainEntry->y, mainEntry->z, mainEntry->w);
            printf("mirror-cell %d=%d,%d,%d,%d\n", cell, entry->x,
                   entry->y, entry->z, entry->w);
        }
        for (cell = 0; cell < 32; cell++) {
            printf("visible-mask %d=%08x\n", cell,
                   (unsigned)g_MainVisibleCellMask[cell]);
            printf("mirror-mask %d=%08x\n", cell,
                   (unsigned)g_MirrorVisibleCellMask[cell]);
        }
    }
    if (RageRuntimeConfigEnabled("checks.save_roundtrip", "RAGE_PORT_SMOKE_SAVE_ROUNDTRIP")) {
        GameSaveHeaderRow header = {0};
        const int marker = 123456789;

        BiosBuInit();
        if (g_RaceProgress == NULL) {
            fprintf(stderr, "save roundtrip has no active progress slot\n");
            return EXIT_FAILURE;
        }
        g_RaceProgress->money.value = marker;
        if (!WriteMemoryCardSaveSlot(0, &header)) {
            fprintf(stderr, "save roundtrip write failed\n");
            return EXIT_FAILURE;
        }
        g_RaceProgress->money.value = 0;
        if (!LoadMemoryCardSaveSlot(0, &header) ||
            g_RaceProgress->money.value != marker) {
            fprintf(stderr, "save roundtrip load failed: money=%d\n",
                    g_RaceProgress->money.value);
            return EXIT_FAILURE;
        }
        printf("save roundtrip ok: money=%d\n", g_RaceProgress->money.value);
    }
    if (RageRuntimeConfigEnabled("checks.complete_save_load", "RAGE_PORT_SMOKE_COMPLETE_SAVE_LOAD")) {
        GameSaveHeaderRow header = {0};
        int table;
        int car;
        CarEntry *tables[3] = {
            g_GrandPrixCars, g_ExtraGrandPrixCars, g_TimeAttackCars
        };

        BiosBuInit();
        if (!LoadMemoryCardSaveSlot(0, &header) ||
            g_ExtraGrandPrixUnlocked != 1 ||
            g_MaxClassReached[0] != 4 || g_MaxClassReached[1] != 5 ||
            g_GrandPrixSave.money.value != 999999999 ||
            g_ExtraGrandPrixSave.money.value != 999999999) {
            fprintf(stderr, "complete generated save failed progression load\n");
            return EXIT_FAILURE;
        }
        for (table = 0; table < 3; table++) {
            for (car = 0; car < 13; car++) {
                if (tables[table][car].enabled != 1) {
                    fprintf(stderr,
                            "complete generated save missing table %d car %d\n",
                            table, car);
                    return EXIT_FAILURE;
                }
            }
        }
        printf("complete generated save loaded: classes=4/5 cars=13/13/13\n");
    }
    if (!RageWriteCapturedFrame(
            RageRuntimeConfigGetLegacy("capture.path", "RAGE_PORT_CAPTURE_PATH"))) {
        fprintf(stderr, "failed to capture smoke frame\n");
        return EXIT_FAILURE;
    }
    printf("Rage Racer smoke stopped at frame %d, scene %d, frontend %d, "
           "player=(%d,%d) speed=%d accelerator=%d held=%04x accel_mask=%04x "
           "pad_type=%02x race_phase=%d progress=%d lap=%d gp_class=%d "
           "gp_round=%d class_done=%d series_done=%d gear=%d manual=%d "
           "rpm=%d jitter=%d "
           "terrain_second=%llu terrain_child_reject=%llu "
           "terrain_child_second=%llu model_backface=%llu mirror_y=%d "
           "steer=%d course_mirror=%d retire_camera=%d capture_faces=%d\n",
           g_FrameCounter, g_SceneId, g_FrontendState,
           g_PlayerCar.x, g_PlayerCar.z, g_PlayerCar.speed,
           g_PlayerCar.drive.acceleratorInput.value, g_PadHeld,
           g_PadButtonMapping[2], g_PadType, g_RacePhase,
           g_PlayerCar.trackProgress, g_PlayerCar.lap, g_GrandPrixClass,
           g_GrandPrixRound, g_ClassCompleted, g_SeriesCleared,
           g_PlayerCar.drive.gear, g_PlayerCar.drive.manual,
           g_EngineRpm, g_EngineRpmJitter,
           g_RageTerrainSecondTriangleVisible,
           g_RageTerrainChildRejectBackface,
           g_RageTerrainChildSecondTriangleVisible,
           g_RageModelRejectBackface, g_MirrorPanelY,
           g_PlayerCar.drive.steerPos, g_MirrorMode,
           RageRetireCameraActive(), RageCaptureCurrent()->faceCount);
    printf("memory card: phase=%d status=%d files=%d free=%d mask=%x page=%d\n",
           g_McMenuPhase, g_McStatusResult, g_McCardFileCount,
           g_McFreeBlocks, g_McSlotUsedMask, g_McMenuPage);
    return EXIT_SUCCESS;
}
