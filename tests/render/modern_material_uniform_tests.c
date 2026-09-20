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
    ModernMaterialUniformBuild(&material, 2, &uniform);
    assert(uniform.surface[3] == 2.0f);
    material.shading = RAGE_RENDER_MATERIAL_SHADING_UNLIT;
    ModernMaterialUniformBuild(&material, 0, &uniform);
    assert(uniform.emissiveAndShading[3] == 0.0f && uniform.surface[3] == 0.0f);
    material.shading = (RageRenderMaterialShading)99;
    ModernMaterialUniformBuild(&material, 0, &uniform);
    assert(uniform.emissiveAndShading[3] == -1.0f);
    RenderMeshInstance car = {0};
    car.assetSet = RAGE_RENDER_ASSET_MODEL_BANK;
    car.assetKey = 68;
    car.lamps.headlights = 1;
    ModernMaterialUniformLamps(&car, 3, &uniform);
    assert(uniform.lamps[0].emission[0] == 2.5f);
    assert(uniform.lamps[1].bounds[0] == 85.0f / 256);
    ModernMaterialUniformBuild(&material, 0, &uniform);
    car.component = 2;
    ModernMaterialUniformLamps(&car, 3, &uniform);
    assert(uniform.lamps[0].emission[0] == 0);
    car.component = 0;
    car.lamps.headlights = 0;
    car.lamps.stop = 1;
    ModernMaterialUniformLamps(&car, 3, &uniform);
    assert(uniform.lamps[0].emission[0] == 0);
    return 0;
}
