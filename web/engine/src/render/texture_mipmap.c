#include "texture_mipmap.h"

#include <limits.h>

static uint32_t MipDimension(uint32_t value, uint32_t level) {
    while (level-- != 0 && value > 1) value >>= 1;
    return value != 0 ? value : 1;
}

static int MipPixelCount(uint32_t width, uint32_t height, size_t *pixels) {
    if (pixels == NULL || width == 0 || height == 0 ||
        (size_t)width > SIZE_MAX / (size_t)height) {
        return 0;
    }
    *pixels = (size_t)width * (size_t)height;
    return 1;
}

uint32_t TextureMipLevelCount(uint32_t width, uint32_t height,
                                  uint32_t maximumLevels) {
    uint32_t levels = 0;
    if (width == 0 || height == 0 || maximumLevels == 0) return 0;
    while (levels < maximumLevels) {
        levels++;
        if (width == 1 && height == 1) break;
        if (width > 1) width >>= 1;
        if (height > 1) height >>= 1;
    }
    return levels;
}

size_t TextureMipLevelOffsetRGBA8(uint32_t width, uint32_t height,
                                      uint32_t level) {
    size_t offset = 0;
    uint32_t index;
    if (width == 0 || height == 0) return SIZE_MAX;
    for (index = 0; index < level; index++) {
        size_t pixels;

        if (!MipPixelCount(width, height, &pixels) ||
            pixels > (SIZE_MAX - offset) / 4u) return SIZE_MAX;
        offset += pixels * 4u;
        if (width > 1) width >>= 1;
        if (height > 1) height >>= 1;
    }
    return offset;
}

size_t TextureMipChainSizeRGBA8(uint32_t width, uint32_t height,
                                    uint32_t levels) {
    size_t offset, pixels;
    uint32_t lastWidth, lastHeight;
    if (levels == 0) return 0;
    offset = TextureMipLevelOffsetRGBA8(width, height, levels - 1);
    if (offset == SIZE_MAX) return 0;
    lastWidth = MipDimension(width, levels - 1);
    lastHeight = MipDimension(height, levels - 1);
    if (!MipPixelCount(lastWidth, lastHeight, &pixels) ||
        pixels > (SIZE_MAX - offset) / 4u) return 0;
    return offset + pixels * 4u;
}

int TextureBuildMipChainRGBA8(const uint8_t *source,
                                  uint32_t width, uint32_t height,
                                  uint32_t levels,
                                  uint8_t *destination,
                                  size_t destinationSize) {
    size_t needed;
    uint32_t x, y, channel, level;
    if (source == NULL || destination == NULL || width == 0 || height == 0 ||
        levels == 0 || levels > TextureMipLevelCount(width, height, levels))
        return 0;
    needed = TextureMipChainSizeRGBA8(width, height, levels);
    if (needed == 0 || destinationSize < needed) return 0;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            size_t pixel = ((size_t)y * width + x) * 4u;
            uint32_t alpha = source[pixel + 3];
            destination[pixel + 0] =
                (uint8_t)((source[pixel + 0] * alpha + 127u) / 255u);
            destination[pixel + 1] =
                (uint8_t)((source[pixel + 1] * alpha + 127u) / 255u);
            destination[pixel + 2] =
                (uint8_t)((source[pixel + 2] * alpha + 127u) / 255u);
            destination[pixel + 3] = (uint8_t)alpha;
        }
    }

    for (level = 1; level < levels; level++) {
        uint32_t sourceWidth = MipDimension(width, level - 1);
        uint32_t sourceHeight = MipDimension(height, level - 1);
        uint32_t targetWidth = MipDimension(width, level);
        uint32_t targetHeight = MipDimension(height, level);
        size_t sourceOffset = TextureMipLevelOffsetRGBA8(
            width, height, level - 1);
        size_t targetOffset = TextureMipLevelOffsetRGBA8(
            width, height, level);
        for (y = 0; y < targetHeight; y++) {
            for (x = 0; x < targetWidth; x++) {
                size_t target = targetOffset +
                    ((size_t)y * targetWidth + x) * 4u;
                for (channel = 0; channel < 4; channel++) {
                    uint32_t total = 0, samples = 0;
                    uint32_t sy, sx;
                    uint32_t sourceYBegin =
                        (uint32_t)(((uint64_t)y * sourceHeight) /
                                   targetHeight);
                    uint32_t sourceYEnd =
                        (uint32_t)(((uint64_t)(y + 1u) * sourceHeight) /
                                   targetHeight);
                    uint32_t sourceXBegin =
                        (uint32_t)(((uint64_t)x * sourceWidth) / targetWidth);
                    uint32_t sourceXEnd =
                        (uint32_t)(((uint64_t)(x + 1u) * sourceWidth) /
                                   targetWidth);
                    for (sy = sourceYBegin; sy < sourceYEnd; sy++) {
                        for (sx = sourceXBegin; sx < sourceXEnd; sx++) {
                            size_t at;
                            at = sourceOffset +
                                ((size_t)sy * sourceWidth + sx) * 4u;
                            total += destination[at + channel];
                            samples++;
                        }
                    }
                    destination[target + channel] =
                        (uint8_t)((total + samples / 2u) / samples);
                }
            }
        }
    }
    return 1;
}
