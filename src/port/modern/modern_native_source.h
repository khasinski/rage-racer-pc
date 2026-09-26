#ifndef MODERN_NATIVE_SOURCE_H
#define MODERN_NATIVE_SOURCE_H
#include "render/render_world.h"
#include "render/rmesh.h"
#include "render/render_material.h"
#include "modern_asset_image.h"

/* External resource scope. Image pixels are released through freeImage.
 * Failed image loads preserve outputs. Owner identifies immutable geometry;
 * textureRevision must change when texture content changes. Missing resources
 * never consult the global asset provider. */
typedef struct ModernNativeSource {
    void *context;
    const void *owner;
    uint64_t textureRevision;
    void *(*retain)(void *context);
    void (*release)(void *context);
    const RageRuntimeMesh *(*mesh)(void *context, const RenderMeshInstance *instance);
    int (*material)(void *context, const RenderMeshInstance *instance, uint32_t material,
                    RageRenderMaterial *definition, ModernAssetImage *image);
    int (*sky)(void *context, ModernAssetImage *image);
    void (*freeImage)(ModernAssetImage *image);
} ModernNativeSource;
void ModernNativeGpuPrepareSource(const RenderWorld *world, float aspect,
                                  const ModernNativeSource *source);
#endif
