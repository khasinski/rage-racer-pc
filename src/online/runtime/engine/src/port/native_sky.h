#ifndef PORT_NATIVE_SKY_H
#define PORT_NATIVE_SKY_H
#include "native_texture.h"
#include "sky_panorama_layout.h"

/* Caller supplies 512x256 RGBA storage and explicit immutable sources.
 * TextureImage may overlay a borrowed animated palette. Invalid input or
 * allocation failure preserves output. Source/output must not overlap. */
int DecodeSky(const TextureImage *source,
                    const RageSkyPanoramaLayout *layout, uint8_t *rgba, size_t size);
#endif
