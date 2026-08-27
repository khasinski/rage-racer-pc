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

void RageRenderMaterialDefault(RageRenderMaterial *material);

/* Resolve one material and gameplay texture variant from a rage-rmat v4-v6
 * sidecar. v4/v5 acquire neutral properties, keeping old caches usable. */
int RageRenderMaterialParse(const void *bytes, size_t size,
                            uint32_t materialIndex, uint32_t variant,
                            RageRenderMaterial *material);

#endif
