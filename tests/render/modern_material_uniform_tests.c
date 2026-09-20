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
    ModernMaterialUniformBuild(&material, 0, &uniform);
    car.component = 0;
    car.lamps = (CarLights){1, 0.2f, 0, 1};
    ModernMaterialUniformLamps(&car, 1, &uniform);
    assert(uniform.lamps[0].emission[0] == 0.5f);
    assert(uniform.lamps[0].emission[3] == 1);
    assert(uniform.lamps[1].emission[0] == 0); /* Mirrored UV isn't double lit. */
    ModernMaterialUniformBuild(&material, 0, &uniform);
    car.lamps.stop = 1;
    ModernMaterialUniformLamps(&car, 1, &uniform);
    assert(uniform.lamps[0].emission[0] == 2.5f);
    ModernMaterialUniformBuild(&material, 0, &uniform);
    car.component = 0;
    car.lamps.headlights = 0;
    car.lamps.stop = 1;
    ModernMaterialUniformLamps(&car, 3, &uniform);
    assert(uniform.lamps[0].emission[0] == 0);
    ModernMaterialUniformBuild(&material, 0, &uniform);
    car.assetSet = RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1;
    car.assetKey = 128;
    car.lamps.headlights = 1;
    car.entity = 2;
    ModernMaterialUniformLamps(&car, 7, &uniform);
    assert(uniform.lamps[0].emission[0] == 2.5f);
    assert(uniform.lamps[1].emission[0] == 0);
    ModernMaterialUniformBuild(&material, 0, &uniform);
    car.entity = 11; /* Same rival model driven by player in Custom Race. */
    ModernMaterialUniformLamps(&car, 7, &uniform);
    assert(uniform.lamps[0].emission[0] == 2.5f);
    for (int driver = 0; driver < 2; ++driver) {
        car.entity = driver ? 11 : 2;
        car.lamps = (CarLights){0, 0.2f, 0, 1};
        ModernMaterialUniformBuild(&material, 0, &uniform);
        ModernMaterialUniformLamps(&car, 5, &uniform);
        assert(uniform.lamps[0].emission[0] == 0.5f);
        assert(uniform.lamps[0].emission[3] == 1);
        assert(uniform.lamps[1].emission[0] == 0);
        car.lamps.stop = 1;
        ModernMaterialUniformBuild(&material, 0, &uniform);
        ModernMaterialUniformLamps(&car, 5, &uniform);
        assert(uniform.lamps[0].emission[0] == 2.5f);
        assert(uniform.lamps[1].emission[0] == 0);
    }
    ModernMaterialUniformBuild(&material, 0, &uniform);
    car.mesh = 5;
    car.lamps.headlights = 1;
    ModernMaterialUniformLamps(&car, 7, &uniform);
    assert(uniform.lamps[0].emission[0] == 0);
    for (int driver = 0; driver < 2; ++driver) {
        car.entity = driver ? 11 : 2;
        car.lamps = (CarLights){1, 0.2f, 0, 1};
        ModernMaterialUniformBuild(&material, 0, &uniform);
        ModernMaterialUniformLamps(&car, 18, &uniform);
        assert(uniform.lamps[0].emission[0] == 2.5f);
        assert(uniform.lamps[1].emission[0] == 0);
        ModernMaterialUniformBuild(&material, 0, &uniform);
        ModernMaterialUniformLamps(&car, 15, &uniform);
        assert(uniform.lamps[0].emission[0] == 0.5f);
        assert(uniform.lamps[1].emission[0] == 0);
        car.lamps.stop = 1;
        ModernMaterialUniformBuild(&material, 0, &uniform);
        ModernMaterialUniformLamps(&car, 15, &uniform);
        assert(uniform.lamps[0].emission[0] == 2.5f);
    }
    for (int driver = 0; driver < 2; ++driver) {
        car.entity = driver ? 11 : 2;
        for (unsigned mesh = 10; mesh <= 15; mesh += 5) {
            car.mesh = mesh;
            car.lamps = (CarLights){1, 0.2f, 0, 1};
            unsigned rearMaterial = mesh == 10 ? 23 : 28;
            unsigned rearPatches = mesh == 10 ? 1 : 4;
            ModernMaterialUniformBuild(&material, 0, &uniform);
            ModernMaterialUniformLamps(&car, mesh == 10 ? 24 : 31, &uniform);
            assert(uniform.lamps[0].emission[0] == 2.5f);
            assert(uniform.lamps[1].emission[0] == 2.5f);
            ModernMaterialUniformBuild(&material, 0, &uniform);
            ModernMaterialUniformLamps(&car, rearMaterial, &uniform);
            for (unsigned i = 0; i < rearPatches; ++i)
                assert(uniform.lamps[i].emission[0] == 0.5f);
            assert(uniform.lamps[rearPatches].emission[0] == 0);
            car.lamps.stop = 1;
            ModernMaterialUniformBuild(&material, 0, &uniform);
            ModernMaterialUniformLamps(&car, rearMaterial, &uniform);
            for (unsigned i = 0; i < rearPatches; ++i)
                assert(uniform.lamps[i].emission[0] == 2.5f);
        }
    }
    car.mesh = 4;
    for (unsigned bank = 88; bank <= 92; bank += 2) {
        const unsigned front[] = {0, 5, 8, 11, 14, 19, 23};
        const unsigned rear[] = {1, 6, 9, 12, 15, 20, 24};
        car.assetKey = bank;
        for (unsigned model = 0; model < 7; ++model) {
            car.mesh = model * 5;
            car.lamps = (CarLights){1, 0.2f, 0, 1};
            ModernMaterialUniformBuild(&material, 0, &uniform);
            ModernMaterialUniformLamps(&car, front[model], &uniform);
            assert(uniform.lamps[0].emission[0] == 2.5f);
            assert(uniform.lamps[1].emission[0] == 2.5f);
            ModernMaterialUniformBuild(&material, 0, &uniform);
            ModernMaterialUniformLamps(&car, rear[model], &uniform);
            assert(uniform.lamps[0].emission[0] == 0.5f);
            assert(uniform.lamps[1].emission[0] == 0.5f);
            car.lamps.stop = 1;
            ModernMaterialUniformBuild(&material, 0, &uniform);
            ModernMaterialUniformLamps(&car, rear[model], &uniform);
            assert(uniform.lamps[0].emission[0] == 2.5f);
            assert(uniform.lamps[1].emission[0] == 2.5f);
        }
    }
    for (unsigned bank = 96; bank <= 100; bank += 2)
    for (unsigned model = 0; model < 7; ++model) {
        const unsigned meshes[] = {0, 5, 10, 15, 20, 25, 30};
        const unsigned front[] = {0, 5, 10, 14, 17, 22, 26};
        const unsigned rear[] = {1, 6, 11, 15, 18, 23, 27};
        car.assetKey = bank;
        car.mesh = meshes[model];
        car.lamps = (CarLights){1, 0.2f, 0, 1};
        ModernMaterialUniformBuild(&material, 0, &uniform);
        ModernMaterialUniformLamps(&car, front[model], &uniform);
        assert(uniform.lamps[0].emission[0] == 2.5f);
        assert(uniform.lamps[1].emission[0] == 2.5f);
        ModernMaterialUniformBuild(&material, 0, &uniform);
        ModernMaterialUniformLamps(&car, rear[model], &uniform);
        assert(uniform.lamps[0].emission[0] == 0.5f);
        assert(uniform.lamps[1].emission[0] == 0.5f);
        car.lamps.stop = 1;
        ModernMaterialUniformBuild(&material, 0, &uniform);
        ModernMaterialUniformLamps(&car, rear[model], &uniform);
        assert(uniform.lamps[0].emission[0] == 2.5f);
        ModernMaterialUniformBuild(&material, 0, &uniform);
        ModernMaterialUniformLamps(&car, model ? front[model] - 3 : 99, &uniform);
        assert(uniform.lamps[0].emission[0] == 0);
        ModernMaterialUniformLamps(&car, model ? 0 : 99, &uniform);
        assert(uniform.lamps[0].emission[0] == 0);
    }
    car.assetKey = 128;
    car.mesh = 4;
    car.lamps = (CarLights){1, 0.2f, 0, 1};
    ModernMaterialUniformBuild(&material, 0, &uniform);
    ModernMaterialUniformLamps(&car, 0, &uniform);
    assert(uniform.lamps[0].emission[0] == 0.5f);
    assert(uniform.lamps[1].emission[0] == 0.5f);
    assert(uniform.lamps[2].emission[0] == 0);
    assert(uniform.lamps[0].bounds[0] == 178.0f / 256);
    ModernMaterialUniformBuild(&material, 0, &uniform);
    ModernMaterialUniformLamps(&car, 5, &uniform);
    assert(uniform.lamps[0].emission[0] == 0);
    car.lamps.stop = 1;
    ModernMaterialUniformBuild(&material, 0, &uniform);
    ModernMaterialUniformLamps(&car, 0, &uniform);
    assert(uniform.lamps[0].emission[0] == 2.5f);
    assert(uniform.lamps[1].emission[0] == 2.5f);
    /* The compact's stop lights use the rear atlas, not its headlights.
     * Their upper red sections brighten together and turn off in daylight. */
    car.assetSet = RAGE_RENDER_ASSET_MODEL_BANK;
    car.assetKey = 10;
    car.mesh = 0;
    car.component = 0;
    car.lamps = (CarLights){0, 0.2f, 0, 1};
    ModernMaterialUniformBuild(&material, 0, &uniform);
    ModernMaterialUniformLamps(&car, 0, &uniform);
    assert(uniform.lamps[0].emission[0] == 0.5f);
    assert(uniform.lamps[1].emission[0] == 0.5f);
    assert(uniform.lamps[0].bounds[3] == 17.0f / 256);
    car.lamps.stop = 1;
    ModernMaterialUniformBuild(&material, 0, &uniform);
    ModernMaterialUniformLamps(&car, 0, &uniform);
    assert(uniform.lamps[0].emission[0] == 2.5f);
    assert(uniform.lamps[1].emission[0] == 2.5f);
    ModernMaterialUniformBuild(&material, 0, &uniform);
    ModernMaterialUniformLamps(&car, 3, &uniform);
    assert(uniform.lamps[0].emission[0] == 0);
    car.lamps = (CarLights){0};
    ModernMaterialUniformBuild(&material, 0, &uniform);
    ModernMaterialUniformLamps(&car, 0, &uniform);
    assert(uniform.lamps[0].emission[0] == 0);
    assert(uniform.lamps[1].emission[0] == 0);
    car.assetKey = 18;
    car.lamps.headlights = 1;
    ModernMaterialUniformBuild(&material, 0, &uniform);
    ModernMaterialUniformLamps(&car, 3, &uniform);
    assert(uniform.lamps[0].emission[0] == 2.5f);
    assert(uniform.lamps[1].emission[0] == 2.5f);
    assert(uniform.lamps[0].emission[3] == 1);
    assert(uniform.lamps[0].bounds[0] > 14.0f / 256);
    assert(uniform.lamps[1].bounds[2] < 82.0f / 256);
    car.lamps = (CarLights){0, 0.2f, 0, 1};
    ModernMaterialUniformBuild(&material, 0, &uniform);
    ModernMaterialUniformLamps(&car, 0, &uniform);
    assert(uniform.lamps[0].emission[0] == 0.5f);
    assert(uniform.lamps[1].emission[0] == 0.5f);
    car.lamps.stop = 1;
    ModernMaterialUniformBuild(&material, 0, &uniform);
    ModernMaterialUniformLamps(&car, 0, &uniform);
    assert(uniform.lamps[0].emission[0] == 2.5f);
    assert(uniform.lamps[1].emission[0] == 2.5f);
    ModernMaterialUniformBuild(&material, 0, &uniform);
    ModernMaterialUniformLamps(&car, 3, &uniform);
    assert(uniform.lamps[0].emission[0] == 0);
    car.assetKey = 12;
    car.lamps = (CarLights){1, 0.2f, 0, 1};
    ModernMaterialUniformBuild(&material, 0, &uniform);
    ModernMaterialUniformLamps(&car, 3, &uniform);
    assert(uniform.lamps[0].emission[0] == 0); /* Base model's material isn't a lamp. */
    ModernMaterialUniformLamps(&car, 4, &uniform);
    assert(uniform.lamps[0].emission[0] == 2.5f);
    assert(uniform.lamps[1].emission[0] == 2.5f);
    ModernMaterialUniformBuild(&material, 0, &uniform);
    ModernMaterialUniformLamps(&car, 0, &uniform);
    assert(uniform.lamps[0].bounds[1] == 205.0f / 256);
    assert(uniform.lamps[0].emission[0] == 0.5f);
    car.assetKey = 14;
    ModernMaterialUniformBuild(&material, 0, &uniform);
    ModernMaterialUniformLamps(&car, 4, &uniform);
    assert(uniform.lamps[0].bounds[0] == 5.0f / 256);
    assert(uniform.lamps[0].emission[0] == 2.5f);
    car.assetKey = 16;
    ModernMaterialUniformBuild(&material, 0, &uniform);
    ModernMaterialUniformLamps(&car, 0, &uniform);
    assert(uniform.lamps[0].bounds[0] == 244.0f / 256);
    assert(uniform.lamps[0].emission[3] == 1);
    assert(uniform.lamps[1].emission[0] == 0.5f);
    ModernMaterialUniformBuild(&material, 0, &uniform);
    ModernMaterialUniformLamps(&car, 4, &uniform);
    assert(uniform.lamps[0].emission[0] == 0);
    ModernMaterialUniformLamps(&car, 3, &uniform);
    assert(uniform.lamps[0].emission[0] == 2.5f);
    car.assetKey = 46;
    ModernMaterialUniformBuild(&material, 0, &uniform);
    ModernMaterialUniformLamps(&car, 3, &uniform);
    assert(uniform.lamps[0].emission[0] == 0); /* Closed pop-up covers. */
    ModernMaterialUniformLamps(&car, 1, &uniform);
    assert(uniform.lamps[0].emission[0] == 2.5f);
    assert(uniform.lamps[1].emission[0] == 2.5f);
    car.assetKey = 72;
    car.lamps.stop = 1;
    ModernMaterialUniformBuild(&material, 0, &uniform);
    ModernMaterialUniformLamps(&car, 0, &uniform);
    assert(uniform.lamps[0].emission[0] == 2.5f);
    assert(uniform.lamps[1].emission[0] == 0); /* Continuous strip emitted once. */
    return 0;
}
