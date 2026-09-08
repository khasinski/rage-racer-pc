#include "port/modern/modern_png.h"
#include <assert.h>
#include <string.h>

static void WriteBE32(Uint8 *bytes, Uint32 value) {
    bytes[0] = (Uint8)(value >> 24);
    bytes[1] = (Uint8)(value >> 16);
    bytes[2] = (Uint8)(value >> 8);
    bytes[3] = (Uint8)value;
}

int main(void) {
    SDL_Surface *source = SDL_CreateSurface(4, 3, SDL_PIXELFORMAT_RGBA32);
    SDL_IOStream *output = SDL_IOFromDynamicMem();
    assert(source && output);
    assert(SDL_FillSurfaceRect(source, NULL, 0x12345678));
    assert(SDL_SavePNG_IO(source, output, false));
    const void *encoded = SDL_GetPointerProperty(SDL_GetIOProperties(output),
        SDL_PROP_IOSTREAM_DYNAMIC_MEMORY_POINTER, NULL);
    size_t size = (size_t)SDL_GetIOSize(output);
    assert(encoded && size > 33);
    SDL_Surface *loaded = ModernLoadPNG(SDL_IOFromConstMem(encoded, size), 4);
    assert(loaded && loaded->w == 4 && loaded->h == 3);
    SDL_DestroySurface(loaded);
    /* This is a complete, decodable PNG. The smaller policy limit must reject
     * it in preflight, before calling the decoder, rather than after decode. */
    assert(!ModernLoadPNG(SDL_IOFromConstMem(encoded, size), 3));
    assert(strstr(SDL_GetError(), "texture limit"));

    /* Failure after a valid header still closes the decoder-owned stream. */
    assert(!ModernLoadPNG(SDL_IOFromConstMem(encoded, 33), 16384));
    int allocations = SDL_GetNumAllocations();
    for (unsigned i = 0; i < 100; i++) {
        assert(!ModernLoadPNG(SDL_IOFromConstMem(encoded, size), 3));
        assert(!ModernLoadPNG(SDL_IOFromConstMem(encoded, 33), 16384));
    }
    assert(SDL_GetNumAllocations() == allocations);

    Uint8 *copy = SDL_malloc(size + 16);
    assert(copy);
    memcpy(copy, encoded, size);
    const Uint32 invalidDimensions[] = {0, 16385, 0x80000000u, 0xffffffffu};
    for (unsigned axis = 0; axis < 2; axis++) {
        for (unsigned i = 0; i < SDL_arraysize(invalidDimensions); i++) {
            memcpy(copy, encoded, size);
            WriteBE32(copy + 16 + axis * 4, invalidDimensions[i]);
            assert(!ModernLoadPNG(SDL_IOFromConstMem(copy, size), 16384));
            assert(strstr(SDL_GetError(), "texture limit"));
        }
    }
    /* CgBI must not provide a route around the same size guard. */
    memcpy(copy, encoded, 8);
    WriteBE32(copy + 8, 4);
    memcpy(copy + 12, "CgBI", 4);
    memset(copy + 16, 0, 8);
    memcpy(copy + 24, (const Uint8 *)encoded + 8, size - 8);
    WriteBE32(copy + 32, 16385);
    assert(!ModernLoadPNG(SDL_IOFromConstMem(copy, size + 16), 16384));
    assert(strstr(SDL_GetError(), "texture limit"));
    WriteBE32(copy + 8, 0xffffffffu);
    assert(!ModernLoadPNG(SDL_IOFromConstMem(copy, size + 16), 16384));
    assert(strstr(SDL_GetError(), "truncated"));

    for (size_t prefix = 1; prefix < 33; prefix++)
        assert(!ModernLoadPNG(SDL_IOFromConstMem(encoded, prefix), 16384));
    memcpy(copy, encoded, size);
    copy[0] = 0;
    assert(!ModernLoadPNG(SDL_IOFromConstMem(copy, size), 16384));
    memcpy(copy, encoded, size);
    WriteBE32(copy + 8, 12);
    assert(!ModernLoadPNG(SDL_IOFromConstMem(copy, size), 16384));
    memcpy(copy + 12, "IDAT", 4);
    assert(!ModernLoadPNG(SDL_IOFromConstMem(copy, size), 16384));
    assert(!ModernLoadPNG(SDL_IOFromConstMem(encoded, size), 0));
    assert(!ModernLoadPNG(NULL, 16384));

    /* A stream can start at a nonzero offset; rewind to that same position. */
    memset(copy, 0, 16);
    memcpy(copy + 16, encoded, size);
    SDL_IOStream *offset = SDL_IOFromConstMem(copy, size + 16);
    assert(offset && SDL_SeekIO(offset, 16, SDL_IO_SEEK_SET) == 16);
    loaded = ModernLoadPNG(offset, 4);
    assert(loaded && loaded->w == 4 && loaded->h == 3);
    SDL_DestroySurface(loaded);
    SDL_free(copy);
    SDL_CloseIO(output);
    SDL_DestroySurface(source);
    SDL_Quit();
    return 0;
}
