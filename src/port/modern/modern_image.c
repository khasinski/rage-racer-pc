#include "modern_image.h"
#include "modern_png.h"
#include <webp/decode.h>
#include <string.h>

SDL_Surface *ModernLoadImage(SDL_IOStream *stream, unsigned limit) {
    Uint8 header[12];
    Sint64 start, size;
    SDL_Surface *surface = NULL;
    void *data = NULL;
    if (!stream) return NULL;
    start = SDL_TellIO(stream);
    size = SDL_GetIOSize(stream) - start;
    if (start < 0 || size < 12 || size > 64 * 1024 * 1024 || !limit ||
        SDL_ReadIO(stream, header, sizeof(header)) != sizeof(header) ||
        SDL_SeekIO(stream, start, SDL_IO_SEEK_SET) != start) goto done;
    if (!memcmp(header, "\x89PNG\r\n\x1a\n", 8))
        return ModernLoadPNG(stream, limit);
    if (memcmp(header, "RIFF", 4) || memcmp(header + 8, "WEBP", 4)) goto done;
    Uint32 riffSize = (Uint32)header[4] | (Uint32)header[5] << 8 |
                      (Uint32)header[6] << 16 | (Uint32)header[7] << 24;
    if ((Uint64)riffSize + 8 != (Uint64)size) goto done;
    data = SDL_malloc((size_t)size);
    if (!data || SDL_ReadIO(stream, data, (size_t)size) != (size_t)size) goto done;
    WebPBitstreamFeatures features;
    if (WebPGetFeatures(data, (size_t)size, &features) != VP8_STATUS_OK ||
        features.has_animation || features.width <= 0 || features.height <= 0 ||
        (unsigned)features.width > limit || (unsigned)features.height > limit)
        goto done;
    surface = SDL_CreateSurface(features.width, features.height, SDL_PIXELFORMAT_RGBA32);
    if (surface && !WebPDecodeRGBAInto(data, (size_t)size, surface->pixels,
            (size_t)surface->pitch * surface->h, surface->pitch)) {
        SDL_DestroySurface(surface);
        surface = NULL;
    }
done:
    SDL_free(data);
    SDL_CloseIO(stream);
    if (!surface) SDL_SetError("Invalid, oversized or unsupported texture image");
    return surface;
}
