#ifndef RAGE_RENDER_MATERIAL_H
#define RAGE_RENDER_MATERIAL_H

#include <stddef.h>
#include <stdint.h>

typedef struct RageRenderMaterialPath {
    const char *text;
    size_t length;
} RageRenderMaterialPath;

typedef enum RageRenderMaterialShading {
    RAGE_RENDER_MATERIAL_SHADING_INHERIT = 0,
    RAGE_RENDER_MATERIAL_SHADING_LIT,
    RAGE_RENDER_MATERIAL_SHADING_UNLIT,
} RageRenderMaterialShading;

typedef enum RageRenderMaterialAlphaMode {
    RAGE_RENDER_MATERIAL_ALPHA_AUTO = 0,
    RAGE_RENDER_MATERIAL_ALPHA_OPAQUE,
    RAGE_RENDER_MATERIAL_ALPHA_MASK,
    RAGE_RENDER_MATERIAL_ALPHA_BLEND,
} RageRenderMaterialAlphaMode;

/* Renderer-neutral material data. Paths refer to the caller-owned sidecar
 * bytes; all numeric properties are conventional linear material inputs. */
typedef struct RageRenderMaterial {
    RageRenderMaterialPath baseColorTexture;
    RageRenderMaterialPath paintMask;
    float baseColorFactor[4];
    float emissiveFactor[3];
    float roughness;
    float metallic;
    RageRenderMaterialShading shading;
    RageRenderMaterialAlphaMode alphaMode;
} RageRenderMaterial;

void RenderMaterialDefault(RageRenderMaterial *material);
int RenderMaterialParseProperties(const char *text, size_t size,
                                      RageRenderMaterial *material);

typedef struct RageMaterialCatalogEntry RageMaterialCatalogEntry;
typedef struct RageMaterialCatalog {
    RageMaterialCatalogEntry *entries;
    size_t count;
    char *bytes;
} RageMaterialCatalog;

/* Initialize with {0}. Open copies and validates the entire v4-v6 sidecar,
 * including all variants, paths and unique material IDs. A failed open leaves
 * an existing catalog intact. A successful open replaces its owned snapshot.
 * Find performs no allocation/I/O. Returned paths live until reopen/release;
 * failure leaves the output unchanged. Independent catalogs share no state. */
int RenderMaterialCatalogOpen(RageMaterialCatalog *catalog,
                              const void *bytes, size_t size);
int RenderMaterialCatalogFind(const RageMaterialCatalog *catalog,
                              uint32_t materialIndex, uint32_t variant,
                              RageRenderMaterial *material);
void RenderMaterialCatalogRelease(RageMaterialCatalog *catalog);

/* Resolve one material and gameplay texture variant from a rage-rmat v4-v6
 * sidecar. v4/v5 acquire neutral properties, keeping old caches usable.
 * Validates the entire source; returned paths borrow the supplied bytes.
 * Repeated consumers should open a catalog once instead. */
int RenderMaterialParse(const void *bytes, size_t size,
                            uint32_t materialIndex, uint32_t variant,
                            RageRenderMaterial *material);

#endif
