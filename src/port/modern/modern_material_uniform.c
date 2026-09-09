#include "modern_material_uniform.h"

#include <string.h>

void ModernMaterialUniformBuild(const RageRenderMaterial *material,
                                int allowClearcoat, ModernMaterialUniform *out) {
    memset(out, 0, sizeof(*out));
    memcpy(out->baseColor, material->baseColorFactor, sizeof(out->baseColor));
    memcpy(out->emissiveAndShading, material->emissiveFactor,
           sizeof(material->emissiveFactor));
    out->emissiveAndShading[3] = material->shading == RAGE_RENDER_MATERIAL_SHADING_LIT ? 1.0f :
        material->shading == RAGE_RENDER_MATERIAL_SHADING_UNLIT ? 0.0f : -1.0f;
    out->surface[0] = material->roughness;
    out->surface[1] = material->metallic;
    out->surface[2] = (float)material->alphaMode;
    out->surface[3] = allowClearcoat ? 1.0f : 0.0f;
}
