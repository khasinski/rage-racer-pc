#include "render/render_scene.h"

static RageRenderScene sceneA;
static RageRenderScene sceneB;

int main(void) {
    uint64_t baseline;

    sceneA.sceneId = sceneB.sceneId = 12;
    sceneA.sceneTimer = sceneB.sceneTimer = 562;
    sceneA.drawCount = sceneB.drawCount = 1;
    sceneA.draws[0].modelIndex = sceneB.draws[0].modelIndex = 3;
    sceneA.draws[0].bankId = 0x1111;
    sceneB.draws[0].bankId = 0x9999;
    baseline = RageRenderSceneHash(&sceneA);
    if (baseline != RageRenderSceneHash(&sceneB)) return 1;
    sceneB.draws[0].modelIndex++;
    if (baseline == RageRenderSceneHash(&sceneB)) return 2;
    sceneB.draws[0].modelIndex--;
    sceneB.sceneTimer++;
    if (baseline == RageRenderSceneHash(&sceneB)) return 3;
    return 0;
}
