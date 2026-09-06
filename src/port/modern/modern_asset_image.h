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
/* Material providers publish tightly packed RGBA8, without row padding. */
static inline int ModernAssetImageValidRGBA(const ModernAssetImage *image) {
    if(!image||!image->pixels||!image->width||!image->height)return 0;
    size_t row=(size_t)image->width*4;
    if(row/4!=image->width)return 0;
    if((size_t)image->height>SIZE_MAX/row)return 0;
    return image->size==row*(size_t)image->height;
}
#endif
