#ifndef RAGE_AUTHORED_CAR_SURFACE_H
#define RAGE_AUTHORED_CAR_SURFACE_H
#include "render_material.h"

/* Authored OBJ slots: original texture slot + 64 * surface. Runtime slots
 * retain the resolved disc/cache texture identity in their low twelve bits. */
enum {
    RAGE_CAR_SURFACE_ORIGINAL, RAGE_CAR_SURFACE_GLASS,
    RAGE_CAR_SURFACE_PAINT, RAGE_CAR_SURFACE_RUBBER, RAGE_CAR_SURFACE_METAL,
    RAGE_CAR_SURFACE_DECAL,
    RAGE_CAR_SURFACE_COUNT,
    RAGE_CAR_SURFACE_SOURCE_STRIDE = 64,
    RAGE_CAR_SURFACE_RUNTIME_STRIDE = 4096
};
void AuthoredCarSurfaceApply(unsigned surface, RageRenderMaterial *material);
/* Surface defaults precede explicit mod properties. Invalid properties leave
 * the caller's material unchanged. Texture paths are retained. */
int AuthoredCarSurfaceResolve(unsigned surface, const char *properties,
                             RageRenderMaterial *material);
void AuthoredCarSurfaceTexture(unsigned surface, uint8_t *rgba, size_t size);
#endif
