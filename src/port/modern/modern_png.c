#include "modern_png.h"
#include <string.h>

static Uint32 ReadBE32(const Uint8 *bytes) {
    return ((Uint32)bytes[0] << 24) | ((Uint32)bytes[1] << 16) |
           ((Uint32)bytes[2] << 8) | bytes[3];
}

SDL_Surface *ModernLoadPNG(SDL_IOStream *stream, unsigned maxDimension) {
    static const Uint8 signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    Uint8 bytes[8];
    Sint64 start, end;
    if (!stream) return NULL;
    start = SDL_TellIO(stream);
    end = SDL_GetIOSize(stream);
    if (start < 0 || end < start || maxDimension == 0 ||
        SDL_ReadIO(stream, bytes, sizeof(bytes)) != sizeof(bytes) ||
        memcmp(bytes, signature, sizeof(bytes))) goto invalid;
    for (;;) {
        Uint32 length;
        Sint64 position;
        if (SDL_ReadIO(stream, bytes, sizeof(bytes)) != sizeof(bytes))
            goto invalid;
        length = ReadBE32(bytes);
        position = SDL_TellIO(stream);
        /* Include CRC in the bounds check; never seek beyond a truncated
         * chunk. SDL's decoder remains responsible for full PNG validation. */
        if (position < 0 || position > end ||
            (Uint64)(end - position) < (Uint64)length + 4) goto invalid;
        if (!memcmp(bytes + 4, "IHDR", 4)) {
            if (length != 13 || SDL_ReadIO(stream, bytes, 8) != 8)
                goto invalid;
            if (!ReadBE32(bytes) || !ReadBE32(bytes + 4) ||
                ReadBE32(bytes) > maxDimension ||
                ReadBE32(bytes + 4) > maxDimension) {
                SDL_SetError("PNG dimensions exceed texture limit or are zero");
                SDL_CloseIO(stream);
                return NULL;
            }
            if (SDL_SeekIO(stream, start, SDL_IO_SEEK_SET) != start)
                goto invalid;
            return SDL_LoadPNG_IO(stream, true);
        }
        /* SDL also supports Apple's CgBI chunk before IHDR. No other chunk
         * may precede the image header in the decoder used by this project. */
        if (memcmp(bytes + 4, "CgBI", 4) ||
            SDL_SeekIO(stream, (Sint64)length + 4, SDL_IO_SEEK_CUR) < 0)
            goto invalid;
    }
invalid:
    SDL_SetError("Invalid or truncated PNG header");
    SDL_CloseIO(stream);
    return NULL;
}
