#ifndef TEXTURE_UPSCALE_CLI_H
#define TEXTURE_UPSCALE_CLI_H

#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render/texture_upscale.h"

static int SourceTexelNearTransparent(const SDL_Surface *src, uint32_t sx,
                                      uint32_t sy) {
    int dy, dx;
    for (dy = -1; dy <= 1; dy++) {
        int ny = (int)sy + dy;
        if (ny < 0 || ny >= src->h) continue;
        const uint8_t *row =
            (const uint8_t *)src->pixels + (size_t)ny * (size_t)src->pitch;
        for (dx = -1; dx <= 1; dx++) {
            int nx = (int)sx + dx;
            if (nx < 0 || nx >= src->w) continue;
            if (row[(size_t)nx * 4u + 3u] == 0) return 1;
        }
    }
    return 0;
}

static uint8_t MaxChannel3(const uint8_t rgb[3]) {
    uint8_t m = rgb[0];
    if (rgb[1] > m) m = rgb[1];
    if (rgb[2] > m) m = rgb[2];
    return m;
}

static uint8_t *SurfaceToTightRGBA(const SDL_Surface *surface) {
    size_t rowBytes;
    uint8_t *buf;
    int y;
    if (!surface || surface->w <= 0 || surface->h <= 0) return NULL;
    rowBytes = (size_t)surface->w * 4u;
    buf = malloc(rowBytes * (size_t)surface->h);
    if (!buf) return NULL;
    for (y = 0; y < surface->h; y++) {
        memcpy(buf + (size_t)y * rowBytes,
               (const uint8_t *)surface->pixels +
                   (size_t)y * (size_t)surface->pitch,
               rowBytes);
    }
    return buf;
}

/* Pre-process a source RGBA PNG before feeding an external AI upscaler:
 * dilates opaque neighbor RGB into transparent texels so neural network
 * convolutions do not bleed black background into cutout silhouettes. */
static int TextureUpscalePrepareCommand(int argc, char **argv) {
    SDL_Surface *srcRaw = NULL, *src = NULL;
    uint8_t *rgba = NULL;
    int result = 1;

    if (argc != 4) return 1;
    srcRaw = SDL_LoadPNG(argv[2]);
    if (!srcRaw) goto done;
    src = SDL_ConvertSurface(srcRaw, SDL_PIXELFORMAT_RGBA32);
    if (!src || src->w <= 0 || src->h <= 0 || src->w > 4096 || src->h > 4096)
        goto done;
    rgba = SurfaceToTightRGBA(src);
    if (!rgba ||
        !TextureUpscaleDilateAlphaRGB(rgba, (uint32_t)src->w, (uint32_t)src->h,
                                      4u) ||
        !WritePng(argv[3], rgba, (uint32_t)src->w, (uint32_t)src->h))
        goto done;
    printf("{\"width\":%d,\"height\":%d}\n", src->w, src->h);
    result = 0;
done:
    if (result) fputs("Invalid texture upscale prepare inputs\n", stderr);
    free(rgba);
    if (src) SDL_DestroySurface(src);
    if (srcRaw) SDL_DestroySurface(srcRaw);
    return result;
}

/* Built-in C edge-directed texture upscaler (2x / 4x). */
static int TextureUpscaleCommand(int argc, char **argv) {
    unsigned long factor = 0, windowW = 0, windowH = 0;
    char *endFactor = NULL;
    SDL_Surface *srcRaw = NULL, *src = NULL;
    uint8_t *inRgba = NULL, *outRgba = NULL;
    RageTextureUpscaleWindow window = {0};
    size_t outSize;
    int result = 1;

    if (argc != 5 && argc != 7) return 1;
    if (!argv[4][0]) return 1;
    factor = strtoul(argv[4], &endFactor, 10);
    if (*endFactor || (factor != 2ul && factor != 4ul)) return 1;
    if (argc == 7) {
        char *endW = NULL, *endH = NULL;
        if (!argv[5][0] || !argv[6][0]) return 1;
        windowW = strtoul(argv[5], &endW, 10);
        windowH = strtoul(argv[6], &endH, 10);
        if (*endW || *endH || ((!windowW) != (!windowH)) || windowW > 256 ||
            windowH > 256)
            return 1;
        window.widthU = (uint16_t)windowW;
        window.heightV = (uint16_t)windowH;
    }
    srcRaw = SDL_LoadPNG(argv[2]);
    if (!srcRaw) goto done;
    src = SDL_ConvertSurface(srcRaw, SDL_PIXELFORMAT_RGBA32);
    if (!src || src->w <= 0 || src->h <= 0 ||
        (long)src->w * (long)factor > 4096 ||
        (long)src->h * (long)factor > 4096)
        goto done;
    inRgba = SurfaceToTightRGBA(src);
    outSize =
        TextureUpscaleSize((uint32_t)src->w, (uint32_t)src->h, (uint32_t)factor);
    outRgba = outSize ? malloc(outSize) : NULL;
    if (!inRgba || !outRgba ||
        !TextureUpscaleRGBA(inRgba, (uint32_t)src->w, (uint32_t)src->h,
                            (uint32_t)factor, &window, outRgba, outSize) ||
        !WritePng(argv[3], outRgba, (uint32_t)src->w * (uint32_t)factor,
                  (uint32_t)src->h * (uint32_t)factor))
        goto done;
    printf("{\"width\":%lu,\"height\":%lu,\"scale\":%lu}\n",
           (unsigned long)src->w * factor, (unsigned long)src->h * factor,
           factor);
    result = 0;
done:
    if (result) fputs("Invalid texture upscale inputs\n", stderr);
    free(outRgba);
    free(inRgba);
    if (src) SDL_DestroySurface(src);
    if (srcRaw) SDL_DestroySurface(srcRaw);
    return result;
}

/* Normalize an upscaled RGBA PNG against its native PS1 source texture:
 * 1. Re-applies the exact source transparency mask (word 0x0000 -> alpha 0,
 *    RGB 0) so cutouts never acquire dark/bright fringes or shadow-map halos.
 * 2. De-fringes opaque silhouette texels adjacent to transparent texels when
 *    an upscaler bled black (0,0,0) into the edge.
 * 3. When a PS1 texture window (windowW, windowH) is supplied, repeats the
 *    canonical interior tile across the output so UV wrapping is seamless. */
static int TextureUpscaleNormalizeCommand(int argc, char **argv) {
    unsigned long windowW = 0, windowH = 0;
    SDL_Surface *srcRaw = NULL, *upRaw = NULL, *src = NULL, *up = NULL;
    uint8_t *out = NULL;
    uint32_t scaleX, scaleY, tileW, tileH, originX, originY, y, x;
    int result = 1;

    if (argc != 5 && argc != 7) return 1;
    if (argc == 7) {
        char *endW = NULL, *endH = NULL;
        if (!argv[5][0] || !argv[6][0]) return 1;
        windowW = strtoul(argv[5], &endW, 10);
        windowH = strtoul(argv[6], &endH, 10);
        if (*endW || *endH || ((!windowW) != (!windowH)) || windowW > 256 ||
            windowH > 256)
            return 1;
    }
    srcRaw = SDL_LoadPNG(argv[2]);
    upRaw = SDL_LoadPNG(argv[3]);
    if (!srcRaw || !upRaw) goto done;
    src = SDL_ConvertSurface(srcRaw, SDL_PIXELFORMAT_RGBA32);
    up = SDL_ConvertSurface(upRaw, SDL_PIXELFORMAT_RGBA32);
    if (!src || !up || src->w <= 0 || src->h <= 0 || up->w < src->w ||
        up->h < src->h || up->w > 4096 || up->h > 4096 ||
        (up->w % src->w) != 0 || (up->h % src->h) != 0)
        goto done;

    scaleX = (uint32_t)(up->w / src->w);
    scaleY = (uint32_t)(up->h / src->h);
    tileW = (uint32_t)windowW * scaleX;
    tileH = (uint32_t)windowH * scaleY;
    originX =
        (windowW > 0 && windowW * 2ul <= (unsigned long)src->w) ? tileW : 0u;
    originY =
        (windowH > 0 && windowH * 2ul <= (unsigned long)src->h) ? tileH : 0u;

    out = malloc((size_t)up->w * (size_t)up->h * 4u);
    if (!out) goto done;

    for (y = 0; y < (uint32_t)up->h; y++) {
        uint32_t sampleY =
            (tileH > 0 && originY + tileH <= (uint32_t)up->h)
                ? (originY + (y % tileH))
                : y;
        uint32_t srcY = sampleY / scaleY;
        const uint8_t *srcRow =
            (const uint8_t *)src->pixels + (size_t)srcY * (size_t)src->pitch;
        const uint8_t *upRow =
            (const uint8_t *)up->pixels + (size_t)sampleY * (size_t)up->pitch;
        uint8_t *outRow = out + (size_t)y * (size_t)up->w * 4u;
        for (x = 0; x < (uint32_t)up->w; x++) {
            uint32_t sampleX =
                (tileW > 0 && originX + tileW <= (uint32_t)up->w)
                    ? (originX + (x % tileW))
                    : x;
            uint32_t srcX = sampleX / scaleX;
            const uint8_t *sp = srcRow + (size_t)srcX * 4u;
            const uint8_t *upPx = upRow + (size_t)sampleX * 4u;
            uint8_t *dst = outRow + (size_t)x * 4u;
            if (sp[3] == 0) {
                dst[0] = dst[1] = dst[2] = dst[3] = 0;
                continue;
            }
            dst[0] = upPx[0];
            dst[1] = upPx[1];
            dst[2] = upPx[2];
            dst[3] = 255;
            if (SourceTexelNearTransparent(src, srcX, srcY)) {
                uint8_t srcPeak = MaxChannel3(sp);
                uint8_t upPeak = MaxChannel3(dst);
                if (upPeak < srcPeak) {
                    if (upPeak == 0) {
                        dst[0] = sp[0];
                        dst[1] = sp[1];
                        dst[2] = sp[2];
                    } else {
                        for (unsigned c = 0; c < 3; c++) {
                            unsigned scaled =
                                ((unsigned)dst[c] * srcPeak + upPeak / 2u) /
                                upPeak;
                            dst[c] = (uint8_t)(scaled > 255u ? 255u : scaled);
                        }
                    }
                }
            }
        }
    }

    if (!WritePng(argv[4], out, (uint32_t)up->w, (uint32_t)up->h)) goto done;
    printf("{\"width\":%d,\"height\":%d,\"scaleX\":%u,\"scaleY\":%u}\n", up->w,
           up->h, scaleX, scaleY);
    result = 0;
done:
    if (result)
        fputs("Invalid texture upscale normalization inputs\n", stderr);
    free(out);
    if (src) SDL_DestroySurface(src);
    if (up) SDL_DestroySurface(up);
    if (srcRaw) SDL_DestroySurface(srcRaw);
    if (upRaw) SDL_DestroySurface(upRaw);
    return result;
}

#endif
