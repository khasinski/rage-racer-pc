#ifndef RAGE_MODERN_IMAGE_H
#define RAGE_MODERN_IMAGE_H
#include <SDL3/SDL.h>
/* PNG or static WebP. Consumes the seekable stream on every path. Bounds
 * encoded bytes and dimensions before allocating decoded storage. */
SDL_Surface *ModernLoadImage(SDL_IOStream *stream, unsigned maxDimension);
#endif
