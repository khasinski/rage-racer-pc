#ifndef RAGE_TRACK_MATERIAL_PAGE_H
#define RAGE_TRACK_MATERIAL_PAGE_H
#include "render/render_world.h"

/* Palette bits stay independent from the two track-image banks. Car-model
 * variants encode model/paint identity, so they must never be toggled here. */
static inline int TrackMaterialAlternateVariant(RenderAssetSet set,
                                                unsigned variant) {
    if (set == RAGE_RENDER_ASSET_TERRAIN) return (int)(variant ^ 2u);
    if (set == RAGE_RENDER_ASSET_COURSE) return (int)(variant ^ 4u);
    return -1;
}
static inline int TrackMaterialPage(RenderAssetSet set, unsigned variant,
                                    int currentPage) {
    if (set == RAGE_RENDER_ASSET_TERRAIN) return (int)((variant >> 1) & 1u);
    if (set == RAGE_RENDER_ASSET_COURSE) return (int)((variant >> 2) & 1u);
    return currentPage != 0;
}
#endif
