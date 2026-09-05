#include "authored_car_surface.h"

void AuthoredCarSurfaceTexture(unsigned surface, uint8_t *rgba, size_t size) {
    size_t i;
    if (surface != RAGE_CAR_SURFACE_GLASS || !rgba || size % 4) return;
    /* Retail windows paint a different fixed reflection into each UV region.
     * Use one glass tint; the renderer supplies view-dependent reflections.
     * Preserve coverage, including any transparent atlas texels. */
    for (i = 0; i < size; i += 4) {
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
