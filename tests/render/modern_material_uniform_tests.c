#include <assert.h>

#include "modern/modern_material_uniform.h"

int main(void) {
    RageRenderMaterial material = {0};
    ModernMaterialUniform uniform;
    material.baseColorFactor[0] = 0.25f;
    material.emissiveFactor[2] = 0.75f;
    material.roughness = 0.4f;
    material.metallic = 0.8f;
    material.alphaMode = RAGE_RENDER_MATERIAL_ALPHA_BLEND;
    material.shading = RAGE_RENDER_MATERIAL_SHADING_LIT;
    ModernMaterialUniformBuild(&material, 1, &uniform);
    assert(uniform.baseColor[0] == 0.25f && uniform.emissiveAndShading[2] == 0.75f);
    assert(uniform.emissiveAndShading[3] == 1.0f && uniform.surface[0] == 0.4f);
    assert(uniform.surface[1] == 0.8f && uniform.surface[2] == RAGE_RENDER_MATERIAL_ALPHA_BLEND);
    assert(uniform.surface[3] == 1.0f);
    material.shading = RAGE_RENDER_MATERIAL_SHADING_UNLIT;
    ModernMaterialUniformBuild(&material, 0, &uniform);
    assert(uniform.emissiveAndShading[3] == 0.0f && uniform.surface[3] == 0.0f);
    material.shading = (RageRenderMaterialShading)99;
    ModernMaterialUniformBuild(&material, 0, &uniform);
    assert(uniform.emissiveAndShading[3] == -1.0f);
    return 0;
}
