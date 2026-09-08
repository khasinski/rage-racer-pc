#ifndef RAGE_MODERN_PNG_H
#define RAGE_MODERN_PNG_H

#include <SDL3/SDL.h>

/* Consumes and closes stream on both success and failure. Checks dimensions
 * before the decoder allocates pixel storage. The stream must be seekable. */
SDL_Surface *ModernLoadPNG(SDL_IOStream *stream, unsigned maxDimension);

#endif
