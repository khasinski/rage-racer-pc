#ifndef RAGE_MODERN_MATERIAL_UNIFORM_H
#define RAGE_MODERN_MATERIAL_UNIFORM_H

#include "render/render_material.h"

typedef struct ModernMaterialUniform {
    float baseColor[4];
    float emissiveAndShading[4];
    float surface[4];
} ModernMaterialUniform;

void ModernMaterialUniformBuild(const RageRenderMaterial *material,
                                int allowClearcoat, ModernMaterialUniform *out);

#endif
