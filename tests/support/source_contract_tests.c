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
    Require(strstr(gpu, "camera->skyGridOrigin") && strstr(gpu, "camera->skyGridColumn"), "sky ignores classic grid");
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
    free(metal); free(header); free(game); free(gpu); free(dense); free(glsl);
}
int main(int argc, char **argv) {
    if (argc != 3) return 2;
    root = argv[2];
    if (!strcmp(argv[1], "native_car_names")) CarNames();
    else if (!strcmp(argv[1], "architecture_boundaries")) Architecture();
    else if (!strcmp(argv[1], "hud_anchoring")) HudAnchoring();
    else if (!strcmp(argv[1], "native_sky_projection")) Sky();
    else return 2;
    if (!failures) printf("%s passed\n", argv[1]);
    return failures ? 1 : 0;
}
