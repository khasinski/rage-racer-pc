#ifndef RAGE_MODERN_ASSET_IMAGE_H
#define RAGE_MODERN_ASSET_IMAGE_H
#include <stddef.h>
#include <stdint.h>
typedef struct ModernAssetImage {
    void *pixels;
    size_t size;
    uint32_t width;
    uint32_t height;
} ModernAssetImage;
#endif
