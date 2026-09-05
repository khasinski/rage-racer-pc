#include "authored_car_surface.h"

void AuthoredCarSurfaceTexture(unsigned surface, uint8_t *rgba, size_t size) {
    size_t i;
    if (surface == RAGE_CAR_SURFACE_DECAL && rgba && size == 256u * 256u * 4u) {
        /* Isolate the team canvas so filtered mip levels cannot pull in the
         * wheel atlas beside it and leave coloured specks along the hood. */
        for (i = 0; i < size; i += 4) {
            size_t x = (i / 4u) % 256u, y = (i / 4u) / 256u;
            if (x >= 64 && x < 128 && y >= 48 && y < 112) continue;
            rgba[i] = rgba[i + 1] = rgba[i + 2] = rgba[i + 3] = 0;
        }
        return;
    }
    if (surface != RAGE_CAR_SURFACE_GLASS || !rgba || size % 4) return;
    /* Retail windows paint a different fixed reflection into each UV region.
     * Use one glass tint; the renderer supplies view-dependent reflections.
     * Preserve coverage, including any transparent atlas texels. */
    for (i = 0; i < size; i += 4) {
        /* UploadTeamNameTexture writes the 48x8 banner at page-10 texels
         * (8,55). Its glyphs and dark sunstrip belong above the glass tint. */
        if (size == 256u * 256u * 4u) {
            size_t x=(i/4u)%256u, y=(i/4u)/256u;
            if (x>=8 && x<56 && y>=55 && y<63) continue;
        }
        rgba[i] = 18;
        rgba[i + 1] = 25;
        rgba[i + 2] = 32;
    }
}

void AuthoredCarSurfaceApply(unsigned surface, RageRenderMaterial *m) {
    if (!m) return;
    switch (surface) {
    case RAGE_CAR_SURFACE_GLASS:
        m->roughness = 0.08f;
        m->metallic = 0.0f;
        m->baseColorFactor[0] *= 0.65f;
        m->baseColorFactor[1] *= 0.72f;
        m->baseColorFactor[2] *= 0.8f;
        break;
    case RAGE_CAR_SURFACE_PAINT:
    case RAGE_CAR_SURFACE_DECAL:
        m->roughness = 0.38f;
        m->metallic = 0.12f;
        break;
    case RAGE_CAR_SURFACE_RUBBER:
        m->roughness = 0.95f;
        m->metallic = 0.0f;
        break;
    case RAGE_CAR_SURFACE_METAL:
        m->roughness = 0.24f;
        m->metallic = 0.8f;
        break;
    default: break;
    }
}
