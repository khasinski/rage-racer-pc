#include "modern_material_uniform.h"
#include "render/car_lamps.h"

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
    const Lamp *lamps;
    unsigned count = CarLamps(body, &lamps), output = 0;
    for (unsigned i = 0; i < count && output < 8; i++) {
        if (lamps[i].material != material) continue;
        int duplicate = 0;
        for (unsigned j = 0; j < i; j++) {
            if (lamps[j].material == material && lamps[j].kind == lamps[i].kind &&
                lamps[j].round == lamps[i].round &&
                memcmp(lamps[j].bounds, lamps[i].bounds, sizeof(lamps[i].bounds)) == 0)
                duplicate = 1;
        }
        if (duplicate) continue;
        float strength = CarLampIntensity(&body->lamps, lamps[i].kind);
        for (int j = 0; j < 4; j++)
            out->lamps[output].bounds[j] = lamps[i].bounds[j] / 256.0f;
        int front = lamps[i].kind == LAMP_HEAD;
        out->lamps[output].emission[0] = strength * 2.5f;
        out->lamps[output].emission[1] = strength * (front ? 2.35f : 0.025f);
        out->lamps[output].emission[2] = strength * (front ? 2.0f : 0.01f);
        out->lamps[output].emission[3] = (float)lamps[i].round;
        output++;
    }
}
