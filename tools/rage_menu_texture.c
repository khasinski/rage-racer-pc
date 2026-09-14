#include <SDL3/SDL.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum { WIDTH = 112, HEIGHT = 16, PIXELS_PER_WORD = 8 };

static const uint32_t s_texture[] = {
#include "main/PAL/main/menu/assets/custom_menu_texture.inc"
};

static const uint16_t s_palette[16] = {
    0x8000, 0xCE73, 0xC631, 0xC210, 0xBDEF, 0xB9CE, 0xB5AD, 0xB18C,
    0xA529, 0xA108, 0x98C6, 0x94A5, 0x9084, 0x8C63, 0x8842, 0x8421,
};

static uint8_t Component(uint16_t color, int shift) {
    return (uint8_t)(((color >> shift) & 31) * 255 / 31);
}

static int ExportPng(const char *path) {
    SDL_Surface *surface = SDL_CreateSurface(WIDTH, HEIGHT,
                                             SDL_PIXELFORMAT_RGBA32);
    if (surface == NULL) return 0;
    for (int y = 0; y < HEIGHT; y++) {
        uint32_t *row = (uint32_t *)((uint8_t *)surface->pixels +
                                     y * surface->pitch);
        for (int x = 0; x < WIDTH; x++) {
            uint32_t word = s_texture[(y * WIDTH + x) / PIXELS_PER_WORD];
            uint16_t color = s_palette[(word >> ((x & 7) * 4)) & 15];
            row[x] = SDL_MapRGBA(
                SDL_GetPixelFormatDetails(surface->format), NULL,
                Component(color, 0), Component(color, 5),
                Component(color, 10), 255);
        }
    }
    int ok = SDL_SavePNG(surface, path);
    SDL_DestroySurface(surface);
    return ok;
}

static int ClosestPaletteIndex(uint8_t r, uint8_t g, uint8_t b) {
    unsigned bestDistance = UINT32_MAX;
    int best = 0;
    for (int index = 0; index < 16; index++) {
        int dr = r - Component(s_palette[index], 0);
        int dg = g - Component(s_palette[index], 5);
        int db = b - Component(s_palette[index], 10);
        unsigned distance = (unsigned)(dr * dr + dg * dg + db * db);
        if (distance < bestDistance) {
            bestDistance = distance;
            best = index;
        }
    }
    return best;
}

static int ImportPng(const char *input, const char *output) {
    SDL_Surface *loaded = SDL_LoadPNG(input);
    SDL_Surface *surface;
    FILE *file;
    if (loaded == NULL || loaded->w != WIDTH || loaded->h != HEIGHT) {
        fprintf(stderr, "input must be a %dx%d PNG\n", WIDTH, HEIGHT);
        SDL_DestroySurface(loaded);
        return 0;
    }
    surface = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loaded);
    if (surface == NULL) return 0;
    file = fopen(output, "wb");
    if (file == NULL) {
        SDL_DestroySurface(surface);
        return 0;
    }
    for (int wordIndex = 0; wordIndex < WIDTH * HEIGHT / PIXELS_PER_WORD;
         wordIndex++) {
        uint32_t word = 0;
        for (int nibble = 0; nibble < PIXELS_PER_WORD; nibble++) {
            int pixel = wordIndex * PIXELS_PER_WORD + nibble;
            int x = pixel % WIDTH;
            int y = pixel / WIDTH;
            const uint32_t *row = (const uint32_t *)(const void *)(
                (const uint8_t *)surface->pixels + y * surface->pitch);
            uint8_t r, g, b, a;
            SDL_GetRGBA(row[x], SDL_GetPixelFormatDetails(surface->format),
                        NULL, &r, &g, &b, &a);
            word |= (uint32_t)ClosestPaletteIndex(r, g, b) << (nibble * 4);
        }
        fprintf(file, "0x%08x,%s", word,
                wordIndex % 8 == 7 ? "\n" : " ");
    }
    fclose(file);
    SDL_DestroySurface(surface);
    return 1;
}

int main(int argc, char **argv) {
    if ((argc != 3 && argc != 4) ||
        (strcmp(argv[1], "export") && strcmp(argv[1], "import"))) {
        fprintf(stderr, "usage: %s export OUTPUT.png | import INPUT.png [OUTPUT.inc]\n",
                argv[0]);
        return 2;
    }
    if (!strcmp(argv[1], "export")) return ExportPng(argv[2]) ? 0 : 1;
    return ImportPng(
        argv[2], argc == 4 ? argv[3] :
        "src/main/PAL/main/menu/assets/custom_menu_texture.inc") ? 0 : 1;
}
