#include <stdint.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <stdlib.h>
#endif

static int Word(FILE *file, uint32_t value) {
    unsigned char bytes[4] = {(unsigned char)value, (unsigned char)(value >> 8),
        (unsigned char)(value >> 16), (unsigned char)(value >> 24)};
    return fwrite(bytes, 1, 4, file) == 4;
}
static int Float(FILE *file, float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return Word(file, bits);
}
static FILE *Open(const char *root, const char *name) {
    char path[4096];
    if (snprintf(path, sizeof(path), "%s/%s", root, name) >= (int)sizeof(path)) return NULL;
#ifdef _WIN32
    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, NULL, 0);
    if (count <= 0) return NULL;
    wchar_t *wide = malloc((size_t)count * sizeof(*wide));
    if (!wide) return NULL;
    FILE *file = NULL;
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, count))
        file = _wfopen(wide, L"wbx");
    free(wide);
    return file;
#else
    return fopen(path, "wbx");
#endif
}
int main(int argc, char **argv) {
    /* Caller creates an isolated directory and owns cleanup on failure. */
    if (argc != 2) return 1;
    FILE *file = Open(argv[1], "car.rmesh");
    if (!file) return 1;
    int ok = fwrite("RRMESH1\0", 1, 8, file) == 8;
    ok &= Word(file, 1) && Word(file, 1024) && Word(file, 3072) && Word(file, 3072);
    for (unsigned i = 0; i <= 1024; ++i) ok &= Word(file, i * 3);
    for (unsigned mesh = 0; mesh < 1024; ++mesh) {
        float x = (float)(mesh * 4);
        for (unsigned v = 0; v < 3; ++v) {
            ok &= Float(file, x + (v == 0 ? -1.0f : v == 1 ? 1.0f : 0.0f));
            ok &= Float(file, v == 2 ? 2.0f : 0.0f) && Float(file, 0);
            ok &= Float(file, 0) && Float(file, 1) && Float(file, 0);
            ok &= Word(file, UINT32_MAX);
            ok &= Float(file, v == 0 ? 0.0f : v == 1 ? 1.0f : 0.5f);
            ok &= Float(file, v == 2 ? 1.0f : 0.0f) && Word(file, 0);
        }
    }
    for (unsigned i = 0; i < 3072; ++i) ok &= Word(file, i);
    if (fclose(file) || !ok) return 1;
    file = Open(argv[1], "material.rmat"); if (!file) return 1;
    ok = fprintf(file, "# rage-rmat v6\n0 ") > 0;
    for (unsigned i = 0; i < 96; ++i)
        ok &= fprintf(file, "%smaterial.rgba", i ? " " : "") > 0;
    ok &= fprintf(file, " | material.rpaint | inherit auto 1 0 1 1 1 1 0 0 0\n") > 0;
    if (fclose(file) || !ok) return 1;
    file = Open(argv[1], "material.rgba"); if (!file) return 1;
    for (unsigned i = 0; i < 256 * 256; ++i) ok &= Word(file, UINT32_MAX);
    if (fclose(file) || !ok) return 1;
    file = Open(argv[1], "material.rpaint"); if (!file) return 1;
    for (unsigned i = 0; i < 256 * 256; ++i) ok &= fputc(1, file) != EOF;
    if (fclose(file) || !ok) return 1;
    file = Open(argv[1], "runtime-index.txt"); if (!file) return 1;
    ok = fprintf(file, "# rage-rmesh-index v2\n") > 0;
    for (unsigned key = 10; key < 75; ++key)
        ok &= fprintf(file, "%u model car.rmesh material.rmat\n", key) > 0;
    const char *sets[] = {"track-model-1", "track-model-2", "course", "terrain"};
    for (unsigned key = 88; key < 136; key += 2)
        for (unsigned s = 0; s < 4; ++s)
            ok &= fprintf(file, "%u %s car.rmesh material.rmat\n", key, sets[s]) > 0;
    if (fclose(file) || !ok) return 1;
    char pngPath[4096];
    if (snprintf(pngPath, sizeof(pngPath), "%s/terrain.png", argv[1]) >= (int)sizeof(pngPath)) return 1;
    unsigned char pixels[64 * 32 * 4];
    for (unsigned i = 0; i < 64 * 32; ++i) {
        pixels[i*4] = 32; pixels[i*4+1] = 192;
        pixels[i*4+2] = 255; pixels[i*4+3] = 255;
    }
    SDL_Surface *surface = SDL_CreateSurfaceFrom(64, 32, SDL_PIXELFORMAT_RGBA32, pixels, 64 * 4);
    if (!surface) return 1;
    ok = SDL_SavePNG(surface, pngPath);
    SDL_DestroySurface(surface);
    if (!ok) return 1;
    return 0;
}
