#include "authored_car_surface.h"

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
