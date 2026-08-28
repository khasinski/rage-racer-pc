#ifndef RAGE_TEXTURE_MIPMAP_H
#define RAGE_TEXTURE_MIPMAP_H

#include <stddef.h>
#include <stdint.h>

/* PS1 material pages are densely packed atlases. Four levels retain an
 * 8-by-8 source region as at least one independent texel; smaller levels
 * would inevitably combine unrelated atlas entries. */
enum { RAGE_TEXTURE_ATLAS_MIP_LEVELS = 4 };

uint32_t TextureMipLevelCount(uint32_t width, uint32_t height,
                                  uint32_t maximumLevels);
size_t TextureMipLevelOffsetRGBA8(uint32_t width, uint32_t height,
                                      uint32_t level);
size_t TextureMipChainSizeRGBA8(uint32_t width, uint32_t height,
                                    uint32_t levels);

/* Builds a tightly packed, premultiplied-alpha RGBA8 mip chain. Premultiplying
 * before filtering prevents transparent atlas texels from adding dark or
 * incorrectly coloured fringes. */
int TextureBuildMipChainRGBA8(const uint8_t *source,
                                  uint32_t width, uint32_t height,
                                  uint32_t levels,
                                  uint8_t *destination,
                                  size_t destinationSize);

#endif
