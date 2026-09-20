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
    out->surface[3] = (float)allowClearcoat;
}

void ModernMaterialUniformLamps(const RenderMeshInstance *body,
                               uint32_t material, ModernMaterialUniform *out) {
    /* Atlas coordinates are authored against the imported model, before
     * painting. Do not infer lamps from color: several palettes share UVs. */
    if (body->component != 0 || body->assetSet != RAGE_RENDER_ASSET_MODEL_BANK ||
        body->assetKey != 68 || material != 3) return;
    static const float head[2][4] = {
        {12, 131, 28, 139}, {85, 131, 98, 139},
    };
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 4; j++) out->lamps[i].bounds[j] = head[i][j] / 256.0f;
        out->lamps[i].emission[0] = body->lamps.headlights * 2.5f;
        out->lamps[i].emission[1] = body->lamps.headlights * 2.35f;
        out->lamps[i].emission[2] = body->lamps.headlights * 2.0f;
    }
}
