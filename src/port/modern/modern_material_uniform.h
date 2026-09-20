#ifndef RAGE_MODERN_MATERIAL_UNIFORM_H
#define RAGE_MODERN_MATERIAL_UNIFORM_H

#include "render/render_material.h"
#include "render/render_world.h"
#include "render/render_mesh_build.h"

typedef struct ModernMaterialUniform {
    float baseColor[4];
    float emissiveAndShading[4];
    float surface[4];
    struct {
        float bounds[4];
        float emission[4];
    } lamps[8];
} ModernMaterialUniform;

void ModernMaterialUniformBuild(const RageRenderMaterial *material,
                                int allowClearcoat, ModernMaterialUniform *out);
void ModernMaterialUniformLamps(const RenderMeshInstance *body,
                               uint32_t material, ModernMaterialUniform *out);

void ModernMaterialUniformCar(const RenderWorld *world,
                              const RageNativeDrawSpan *span,
                              ModernMaterialUniform *out);

#endif
