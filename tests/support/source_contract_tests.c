/* Compiled replacements for the four source-level Python release checks. */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "source_contract_files.h"

static const char *root;
static int failures;
static void Require(int ok, const char *message) {
    if (!ok) { fprintf(stderr, "FAIL %s\n", message); ++failures; }
}
static char *Read(const char *relative) {
    char path[4096];
    snprintf(path, sizeof(path), "%s/%s", root, relative);
    FILE *file = fopen(path, "rb");
    if (!file) { fprintf(stderr, "cannot read %s\n", path); exit(1); }
    if (fseek(file, 0, SEEK_END) != 0) exit(1);
    long size = ftell(file);
    if (size < 0 || fseek(file, 0, SEEK_SET) != 0) exit(1);
    char *text = malloc((size_t)size + 1);
    if (!text || fread(text, 1, (size_t)size, file) != (size_t)size) exit(1);
    text[size] = 0;
    fclose(file);
    return text;
}
static int Exists(const char *relative) {
    char path[4096]; struct stat info;
    snprintf(path, sizeof(path), "%s/%s", root, relative);
    return stat(path, &info) == 0;
}
static char *Dense(const char *text) {
    char *out = malloc(strlen(text) + 1), *cursor = out;
    if (!out) exit(1);
    for (; *text; ++text) if (!isspace((unsigned char)*text)) *cursor++ = *text;
    *cursor = 0;
    return out;
}
static char *Block(char *text, const char *begin, const char *end) {
    char *start = strstr(text, begin);
    Require(start != NULL, begin);
    if (!start) exit(1);
    start += strlen(begin);
    char *stop = strstr(start, end);
    Require(stop != NULL, end);
    if (!stop) exit(1);
    *stop = 0;
    return start;
}
static void CarNames(void) {
    const char *expected[] = {"ERRISO", "ABEILLE", "PEGASE", "ESPERANZA",
        "ACCERON", "BAYONET", "HIJACK", "FATALITA", "ISTANTE", "GHEPARDO",
        "VAINQURE", "BULSHADE", "SQUALDON"};
    char *source = Read("src/port/native_game_state.c");
    char *copy = Dense(source);
    char *list = Block(copy, "constchar*g_NativeCarNames[GAME_CAR_COUNT]={", "};");
    size_t count = 0;
    for (char *token = strtok(list, ","); token; token = strtok(NULL, ",")) {
        char declaration[256];
        Require(count < sizeof(expected) / sizeof(expected[0]), "too many native car names");
        if (count >= sizeof(expected) / sizeof(expected[0])) break;
        snprintf(declaration, sizeof(declaration), "static char %s[] = \"%s\";", token, expected[count++]);
        Require(strstr(source, declaration) != NULL, declaration);
    }
    Require(count == 13, "native car names must have thirteen retail model entries");
    free(copy); free(source);
}
static void Architecture(void) {
    char *cmake = Read("CMakeLists.txt");
    char *copy = Read("CMakeLists.txt");
    char *sim = Block(copy, "add_library(rage-sim STATIC", ")");
    for (char *p = sim; *p; ++p) *p = (char)tolower((unsigned char)*p);
    Require(!strstr(sim, "psyz"), "rage-sim directly names PSY-Z");
    Require(!Exists("src/main/PAL/lib"), "uncompiled PS1 library sources returned");
    Require(!strstr(cmake, "add_compile_options(-fno-strict-aliasing -fwrapv)"), "legacy flags are global");
    Require(strstr(cmake, "tools/run_lint.cmake") != NULL, "compiled-toolchain lint entry point missing");
    Require(strstr(cmake, "run_lint.py") == NULL, "lint target requires Python");
    char *portConfig = Read("src/port/port_config.c");
    Require(strstr(portConfig, "static RagePortConfig active_config = {") == NULL,
            "active config duplicates first-run defaults");
    Require(strstr(portConfig, "PortConfigDefaults(&active_config)") != NULL,
            "active config does not use the canonical defaults");
    free(portConfig);
    const char *targets[] = {"rage-game-full", "rage-host-state", "rage-port-legacy"};
    for (size_t i = 0; i < 3; ++i) {
        char needle[256];
        snprintf(needle, sizeof(needle), "target_compile_options(%s PRIVATE ${RAGE_COMPAT_COMPILE_OPTIONS})", targets[i]);
        Require(strstr(cmake, needle) != NULL, needle);
    }
    Require(Exists("src/render"), "Render World directory missing");
    Require(Exists("src/port/classic/native_geometry.c"), "classic geometry outside renderer module");
    for (size_t i = 0; neutral_files[i]; ++i) {
        char *text = Read(neutral_files[i]);
        Require(!strstr(text, "<psyz/") && !strstr(text, "\"psyz/") &&
                !strstr(text, "<psyq/") && !strstr(text, "\"psyq/"), neutral_files[i]);
        free(text);
    }
    for (size_t i = 0; modern_files[i]; ++i) {
        char *text = Read(modern_files[i]);
        Require(!strstr(text, "classic/"), modern_files[i]); free(text);
    }
    char *config = Read("src/port/port_config.h");
    int count = 0;
    for (char *p = config; (p = strstr(p, "RAGE_RENDERER_")); p += 14) ++count;
    Require(count == 2 && strstr(config, "RAGE_RENDERER_CLASSIC = 0") &&
            strstr(config, "RAGE_RENDERER_MODERN = 1"), "public renderer identities changed");
    char *modern = Read("src/port/modern/modern_renderer.c");
    const char *forbidden[] = {"ModernNativeGpuCanReplaceWorld", "ModernRenderLegacySelection",
        "MODERN_PIPE_3D", "ModernBuildFaceVertices", "RageCaptureFace"};
    for (size_t i = 0; i < 5; ++i) Require(!strstr(modern, forbidden[i]), forbidden[i]);
    Require(strstr(modern, "ModernBuildOverlayFrame") != NULL, "2D overlay boundary missing");
    Require(strstr(modern, "legacy 3D fallback is disabled") != NULL, "incomplete worlds not diagnosed");
    free(modern); free(config); free(copy); free(cmake);
}
static void HudAnchoring(void) {
    const char *calls[] = {"DrawTimeValue", "DrawMinuteSecondTime", "DrawText8x8"};
    for (size_t i = 0; retail_files[i]; ++i) {
        char *text = Read(retail_files[i]);
        for (size_t j = 0; j < 3; ++j) {
            const char *p = text;
            while ((p = strstr(p, calls[j]))) {
                const char *start = p;
                p += strlen(calls[j]);
                if (start > text && (isalnum((unsigned char)start[-1]) || start[-1] == '_')) continue;
                while (isspace((unsigned char)*p)) ++p;
                if (*p != '(') continue;
                ++p;
                while (isspace((unsigned char)*p)) ++p;
                if (!isdigit((unsigned char)*p)) continue;
                char *end;
                long x = strtol(p, &end, 0);
                if (end == p) continue;
                while (isspace((unsigned char)*end)) ++end;
                if (*end != ',') continue;
                if (x < 80 || x >= 240) {
                    fprintf(stderr, "%s: %s raw edge x=%ld\n", retail_files[i], calls[j], x);
                    ++failures;
                }
            }
        }
        free(text);
    }
}
static void AssetPreparation(void) {
    char *renderer = Read("src/port/modern/modern_renderer.c");
    char *gpu = Read("src/port/modern/modern_native_gpu.c");
    char *assets = Read("src/port/modern/modern_assets.c");
    Require(strstr(renderer, "ModernAssetsPrepareWorld(GameRenderWorldCurrent())") != NULL,
            "logic completion does not prepare the published world");
    Require(strstr(renderer, "ModernAssetsPrepareWorld(GameRenderWorldPrevious())") != NULL,
            "logic-rate presentation world is not prepared");
    Require(strstr(gpu, "ModernAssetsPrepareWorld") == NULL,
            "GPU preparation performs source asset work");
    Require(strstr(gpu, "ModernAssetsResidentMeshLookup") != NULL,
            "GPU preparation does not use resident mesh lookup");
    Require(strstr(assets, "void ModernAssetsPrepareWorld") != NULL,
            "asset preparation boundary is missing");
    free(assets); free(gpu); free(renderer);
}
static void Sky(void) {
    char *glsl = Read("src/port/modern/shaders/native_sky.frag.glsl");
    char *dense = Dense(glsl);
    const char *needles[] = {"cloudBand", "gl_FragCoord.x", "gl_FragCoord.y", "gridOrigin",
        "sky.gridParams.w-gl_FragCoord.y", "(240.0/sky.gridParams.w)",
        "(sky.gridOrigin.w-320.0)*0.5", "cloudCoverage", "mod(-floor(cloudBand),2.0)",
        "atan(direction.x,-direction.z)", "(4.0/tau)", "sky.gridParams.z==1.0", ":1.0",
        "cloudCoverage*=step(0.0,height)*validGrid", "sky.gridParams.z"};
    for (size_t i = 0; i < sizeof(needles)/sizeof(needles[0]); ++i)
        Require(strstr(dense, needles[i]) != NULL, needles[i]);
    Require(!strstr(glsl, "screenPosition"), "sky depends on backend clip-space Y");
    Require(!strstr(glsl, "1.0 - smoothstep(0.528, 0.535, height)"), "fixed-height cloud cutoff");
    char *gpu = Read("src/port/modern/modern_native_gpu.c");
    char *renderer = Read("src/port/modern/modern_renderer.c");
    Require(strstr(gpu, "camera->skyGridOrigin") && strstr(gpu, "camera->skyGridColumn"), "sky ignores classic grid");
    Require(strstr(renderer, "GameRenderWorldPrevious()") != NULL,
            "sky packets do not use their published source world");
    Require(strstr(renderer, "s_skyPacketCamera = &current->previousCamera") == NULL,
            "sky packets use camera-cut-reset history");
    char *stop = strstr(gpu, "s_skySampler = SDL_CreateGPUSampler");
    Require(stop != NULL, "sky sampler missing");
    if (stop) *stop = 0;
    char *sampler = NULL;
    for (char *p = gpu; (p = strstr(p, "s_sampler = SDL_CreateGPUSampler")); ++p) sampler = p;
    Require(sampler != NULL, "base sampler missing");
    if (sampler) {
        Require(strstr(sampler, "address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT") != NULL, "horizontal sky must repeat");
        Require(strstr(sampler, "address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE") != NULL, "vertical sky must clamp");
        Require(strstr(sampler, "min_filter = SDL_GPU_FILTER_NEAREST") != NULL, "sky must use nearest sampling");
    }
    char *game = Read("src/port/render_world_game.c");
    const char *slots[] = {"ENV_SKY_TOP, &camera.skyTopColor", "ENV_SKY_MIDDLE, &camera.skyColor",
        "ENV_SKY_HORIZON, &camera.skyHorizonColor", "ENV_SKY_BOTTOM, &camera.skyBottomColor"};
    for (size_t i = 0; i < 4; ++i) {
        char needle[128]; snprintf(needle, sizeof(needle), "GameRenderWorldEnvironmentColor(%s)", slots[i]);
        Require(strstr(game, needle) != NULL, needle);
    }
    Require(Exists("src/port/modern/shaders/native_sky_frag_spv.h"), "SPIR-V sky missing");
    char *header = Read("src/port/modern/shaders/native_sky_frag_msl.h");
    char *metal = malloc(strlen(header) + 1), *out = metal;
    if (!metal) exit(1);
    for (char *p = header; (p = strstr(p, "0x")); p += 4) {
        unsigned byte;
        if (strlen(p) < 4) break;
        if (sscanf(p + 2, "%2x", &byte) == 1) *out++ = (char)byte;
    }
    *out = 0;
    Require(strstr(metal, "fs_native_sky") != NULL, "Metal sky entry point missing");
    Require(strstr(metal, "cloudBand") != NULL, "Metal sky does not match GLSL cloud band");
    free(metal); free(header); free(game); free(renderer); free(gpu); free(dense); free(glsl);
}
static char *ReadHostState(void) {
    char *all = calloc(1, 1);
    size_t used = 0;
    if (!all) exit(1);
    for (size_t i = 0; host_state_files[i]; ++i) {
        char *text = Read(host_state_files[i]);
        size_t length = strlen(text);
        char *grown = realloc(all, used + length + 2);
        if (!grown) exit(1);
        all = grown;
        memcpy(all + used, text, length);
        used += length;
        all[used++] = '\n';
        all[used] = 0;
        free(text);
    }
    return all;
}
static void NativeFrameOffsets(void) {
    for (size_t i = 0; retail_files[i]; ++i) {
        char *text = Read(retail_files[i]);
        char *dense = Dense(text);
        Require(strstr(dense, "g_DrawBuffer+0xCC") == NULL,
                "PS1 frame-layout offset uses a native pointer");
        Require(strstr(dense, "base+=0xCC") == NULL,
                "PS1 frame-layout offset advances a native pointer");
        free(dense);
        free(text);
    }
}
static void FrameEnvironmentAliases(void) {
    char *display = Read("src/main/PAL/main/render/display_setup.c");
    char *mirror = Read("src/main/PAL/main/render/mirror_pass.c");
    char *tachometer = Read("src/main/PAL/main/render/tachometer_needle.c");
    char *host = ReadHostState();
    Require(strstr(display, "g_DrawEnv1") == NULL &&
            strstr(display, "g_MirrorDrawEnv1") == NULL,
            "second frame environment uses a detached alias");
    Require(strstr(display, "g_FrameContexts[0].environment") != NULL &&
            strstr(display, "g_FrameContexts[1].environment") != NULL,
            "display setup does not update both typed frame contexts");
    const char *mirrorAliases[] = {"g_MirrorDrawEnv0ClipY", "g_MirrorDrawEnv0ClipH",
        "g_MirrorDrawEnv1ClipY", "g_MirrorDrawEnv1ClipH"};
    for (size_t i = 0; i < 4; ++i)
        Require(strstr(mirror, mirrorAliases[i]) == NULL &&
                strstr(host, mirrorAliases[i]) == NULL,
                "mirror clip retains a detached alias");
    Require(strstr(mirror, "g_FrameContexts[0].environment.mirrorDraw.clip.y") &&
            strstr(mirror, "g_FrameContexts[0].environment.mirrorDraw.clip.h") &&
            strstr(mirror, "g_FrameContexts[1].environment.mirrorDraw.clip.y") &&
            strstr(mirror, "g_FrameContexts[1].environment.mirrorDraw.clip.h"),
            "mirror clip does not update both typed owners");
    const char *tachoAliases[] = {"g_TachoNeedlePrim0PageA", "g_TachoNeedlePrim0",
        "g_TachoNeedlePrim1PageA", "g_TachoNeedlePrim1PageB",
        "g_TachoNeedlePrim1", "g_RaceHudSprite11U0"};
    for (size_t i = 0; i < 6; ++i)
        Require(strstr(tachometer, tachoAliases[i]) == NULL &&
                strstr(host, tachoAliases[i]) == NULL,
                "tachometer retains a detached frame alias");
    Require(strstr(tachometer, "&g_FrameContexts[0].layout.raceHud") &&
            strstr(tachometer, "&g_FrameContexts[1].layout.raceHud") &&
            strstr(tachometer, "tachometerDrawModes[0]") &&
            strstr(tachometer, "tachometerDrawModes[1]") &&
            strstr(tachometer, "tachometerFace"),
            "tachometer packets do not use their typed frame owner");
    free(host); free(tachometer); free(mirror); free(display);
}
static void CaptureSync(void) {
    char *source = Read("src/port/main_smoke.c");
    char *function = strstr(source, "int WriteCapturedFrame(const char *path) {");
    char *sync;
    char *drawPage;
    char *frame;
    char *vram;
    Require(function != NULL, "WriteCapturedFrame is missing");
    if (!function) { free(source); return; }
    sync = strstr(function, "DrawSync(0);");
    drawPage = strstr(function, "Psyz_VideoAllocCapturedDrawPage");
    frame = strstr(function, "Psyz_VideoAllocCapturedFrame");
    vram = strstr(function, "Psyz_VideoAllocCapturedVram");
    Require(sync != NULL, "capture does not synchronize submitted ordering tables");
    Require(drawPage != NULL && sync != NULL && sync < drawPage,
            "draw-page download can precede capture synchronization");
    Require(frame != NULL && sync != NULL && sync < frame,
            "front-buffer download can precede capture synchronization");
    Require(vram != NULL && sync != NULL && sync < vram,
            "VRAM download can precede capture synchronization");
    free(source);
}
static void RaceProgress(void) {
    const char *progress[] = {"g_GrandPrixSave", "g_ExtraGrandPrixSave",
                              "g_TimeAttackSave"};
    char *header = Read("include/game/race.h");
    char *host = ReadHostState();
    char *native = Read("src/port/native_game_state.c");
    char *menu = Read("src/main/PAL/main/menu/menu_mode.c");
    char *course = Read("src/main/PAL/main/menu/course_select_screen.c");
    char *car = Read("src/main/PAL/main/menu/car_select_screen.c");
    for (size_t i = 0; i < 3; ++i) {
        char shortDeclaration[128];
        char fullDeclaration[128];
        snprintf(shortDeclaration, sizeof(shortDeclaration), "unsigned char %s[", progress[i]);
        snprintf(fullDeclaration, sizeof(fullDeclaration), "GameRaceProgress %s;", progress[i]);
        Require(strstr(host, shortDeclaration) == NULL,
                "host state truncates a race-progress object");
        Require(strstr(native, fullDeclaration) != NULL,
                "native state lacks a complete race-progress object");
    }
    Require(strstr(header, "sizeof(GameRaceProgress) == 0x14") &&
            strstr(header, "__builtin_offsetof(GameRaceProgress, money) == 0x10"),
            "race-progress layout is not pinned to its retail ABI");
    Require(strstr(header, "#define g_ExtraGrandPrixSaveMaxClass (g_ExtraGrandPrixSave.maxClassReached)") != NULL,
            "Extra GP maximum class is detached from its progress object");
    Require(strstr(menu, "g_GrandPrixSeries = (u16)g_RaceProgress->timeAttackSeries") != NULL,
            "Time Attack does not restore its series from progress");
    for (size_t i = 0; i < 2; ++i) {
        char *source = i == 0 ? course : car;
        Require(strstr(source, "g_RaceProgress->money = g_PlayerMoney") &&
                strstr(source, "g_RaceProgress->timeAttackSeries = g_GrandPrixSeries"),
                "race select does not store the Time Attack series in progress");
    }
    free(car); free(course); free(menu); free(native); free(host); free(header);
}
static void AudioStateLayout(void) {
    char *host = ReadHostState();
    char *pause = Read("src/main/PAL/main/cd/cd_pause_request.c");
    char *track = Read("include/game/track.h");
    char *records = Read("src/main/PAL/main/race/records.c");
    char *reset = Read("src/main/PAL/main/audio/reset_audio_voice_state.c");
    const char *typedState[] = {"s32 g_BestSectorTimes[2][4][3]",
        "s32 g_SectorEndDistance[3]", "s32 g_CarSpecBars[4]",
        "u16 g_TeamLogoClut[16]"};
    Require(strstr(host, "unsigned char g_CdLocResult[8]") != NULL,
            "CdlGetlocP response must remain one eight-byte backing object");
    Require(strstr(host, "unsigned char g_CdLocMinute") == NULL &&
            strstr(host, "unsigned char g_CdLocSecond") == NULL,
            "CdlGetlocP response detaches an MSF byte");
    for (size_t i = 0; i < 4; ++i)
        Require(strstr(host, typedState[i]) != NULL,
                "audio state backing object has the wrong typed dimensions");
    const char *cars[] = {"g_GrandPrixCars", "g_ExtraGrandPrixCars", "g_TimeAttackCars"};
    for (size_t i = 0; i < 3; ++i) {
        char declaration[128];
        snprintf(declaration, sizeof(declaration), "CarEntry %s[GAME_CAR_COUNT]", cars[i]);
        Require(strstr(host, declaration) != NULL,
                "race car state is not one complete typed table");
    }
    Require(strstr(pause, "CdControl(CD_DRIVE_GET_LOCATION, 0, g_CdLocResult)") != NULL &&
            strstr(pause, "g_CdLocResult[2]") != NULL && strstr(pause, "g_CdLocResult[3]") != NULL,
            "pause request does not use the complete GetlocP response");
    Require(strstr(track, "#define g_ChaseYawPrev g_CamPathAngleDelta[CAMPATH_YAW]") != NULL,
            "chase yaw is detached from its retail camera-path alias");
    Require(strstr(records, "g_BestSectorTimes[series][course][slot]") != NULL &&
            strstr(records, "series * RECORD_COURSE_COUNT + course") != NULL &&
            strstr(records, "defaultLapTimes[index]") != NULL,
            "default sector records are not initialized");
    Require(strstr(reset, "ptr[0x78 / 4]") == NULL,
            "audio reset derives channel fields from another global");
    const char *cursor = reset;
    int resets = 0;
    while ((cursor = strstr(cursor, "g_MusicChannels[i].volRight = 0")) != NULL) {
        ++resets;
        ++cursor;
    }
    Require(resets == 1, "audio reset is missing a direct right-volume reset");
    free(reset); free(records); free(track); free(pause); free(host);
}
static int KnownScript(char names[][128], size_t count, const char *name) {
    for (size_t i = 0; i < count; ++i)
        if (!strcmp(names[i], name)) return 1;
    return 0;
}
static void NativeUiScriptLayout(void) {
    char *menu = Read("include/game/menu.h");
    char *internal = Read("include/game/menu_scripts_internal.h");
    char *native = Read("src/port/native_game_state.c");
    char *declarations = malloc(strlen(menu) + strlen(internal) + 1);
    char names[256][128];
    size_t count = 0;
    if (!declarations) exit(1);
    strcpy(declarations, menu);
    strcat(declarations, internal);
    for (size_t i = 0; retail_files[i]; ++i) {
        char *source = Read(retail_files[i]);
        char *cursor = source;
        while ((cursor = strstr(cursor, "RunTimedDrawScript(")) != NULL) {
            char name[128];
            size_t length = 0;
            cursor += strlen("RunTimedDrawScript(");
            while (isspace((unsigned char)*cursor)) ++cursor;
            if (*cursor == '&') ++cursor;
            if (strncmp(cursor, "g_", 2)) continue;
            while ((isalnum((unsigned char)cursor[length]) || cursor[length] == '_') &&
                   length + 1 < sizeof(name)) ++length;
            if (!length) continue;
            memcpy(name, cursor, length);
            name[length] = 0;
            if (!KnownScript(names, count, name)) {
                Require(count < sizeof(names) / sizeof(names[0]), "too many UI scripts");
                if (count < sizeof(names) / sizeof(names[0])) strcpy(names[count++], name);
            }
        }
        free(source);
    }
    for (size_t i = 0; i < count; ++i) {
        char array[192];
        char alias[256];
        char pointer[256];
        const char *shortName = names[i] + 2;
        /* These tokens originate in the bounded names[][128] parser above.
         * State the bounds in the format too, so fortified GCC can verify
         * this source-contract test under -Werror=format-truncation. */
        snprintf(array, sizeof(array), "TimedDrawCommand %.127s[", names[i]);
        snprintf(alias, sizeof(alias), "#define %.127s g_Native%.125s",
                 names[i], shortName);
        snprintf(pointer, sizeof(pointer),
                 "extern const TimedDrawCommand *%.127s;", names[i]);
        Require(strstr(native, array) != NULL || strstr(menu, alias) != NULL ||
                strstr(declarations, pointer) != NULL,
                "serialized PS1 UI script is used as a native command");
    }
    free(declarations); free(native); free(internal); free(menu);
}
int main(int argc, char **argv) {
    if (argc != 3) return 2;
    root = argv[2];
    if (!strcmp(argv[1], "native_car_names")) CarNames();
    else if (!strcmp(argv[1], "architecture_boundaries")) Architecture();
    else if (!strcmp(argv[1], "hud_anchoring")) HudAnchoring();
    else if (!strcmp(argv[1], "native_sky_projection")) Sky();
    else if (!strcmp(argv[1], "asset_preparation")) AssetPreparation();
    else if (!strcmp(argv[1], "native_frame_offsets")) NativeFrameOffsets();
    else if (!strcmp(argv[1], "frame_environment_aliases")) FrameEnvironmentAliases();
    else if (!strcmp(argv[1], "capture_sync")) CaptureSync();
    else if (!strcmp(argv[1], "race_progress")) RaceProgress();
    else if (!strcmp(argv[1], "audio_state_layout")) AudioStateLayout();
    else if (!strcmp(argv[1], "native_ui_script_layout")) NativeUiScriptLayout();
    else return 2;
    if (!failures) printf("%s passed\n", argv[1]);
    return failures ? 1 : 0;
}
