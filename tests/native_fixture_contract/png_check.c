#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    SDL_Surface *loaded = SDL_LoadPNG(argv[1]);
    if (!loaded) return 1;
    SDL_Surface *rgba = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loaded);
    if (!rgba) return 1;
    int ok = rgba->w == 64 && rgba->h == 32;
    if (ok && SDL_LockSurface(rgba)) {
        for (int y = 0; y < rgba->h; ++y) {
            const unsigned char *row = (const unsigned char *)rgba->pixels + (size_t)y * rgba->pitch;
            for (int x = 0; x < rgba->w; ++x) {
                const unsigned char *pixel = row + x * 4;
                if (pixel[0] != 32 || pixel[1] != 192 || pixel[2] != 255 || pixel[3] != 255)
                    ok = 0;
            }
        }
        SDL_UnlockSurface(rgba);
    } else ok = 0;
    SDL_DestroySurface(rgba);
    if (!ok) fputs("Fixture PNG pixels differ from RGBA(32,192,255,255)\n", stderr);
    return ok ? 0 : 1;
}
