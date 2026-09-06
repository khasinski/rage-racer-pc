#include "render/render_world_snapshot.h"

#include <stdio.h>

/* Synthetic inputs only: no retail disc or previously captured marker needed. */
int main(int argc, char **argv) {
    if (argc != 3) return 1;
    RageRenderWorld world;
    RageRenderCamera camera = {0};
    RenderWorldInit(&world, NULL, 0);
    RenderWorldBeginFrame(&world, 1);
    camera.transform.scale = (RageRenderVec3){1, 1, 1};
    camera.transform.orientation.w = 1;
    camera.transform.hasOrientation = 1;
    camera.verticalFovDegrees = 60;
    camera.nearPlane = 1;
    camera.farPlane = 10000;
    camera.skyAssetKey = 96;
    camera.skyColor = (RageRenderVec3){0.2f, 0.4f, 0.6f};
    /* Nondegenerate screen-space cloud grid: loading a panorama alone must
     * not satisfy the test if its geometry prevents it from being visible. */
    camera.skyGridColumn = (RageRenderVec3){64, 0, 0};
    camera.skyGridRow = (RageRenderVec3){0, 128, 0};
    RenderWorldSetCamera(&world, &camera);
    if (!RenderWorldSnapshotWrite(argv[1], &world)) return 1;
    FILE *file = fopen(argv[2], "wb");
    if (file == NULL) return 1;
    int ok = 1;
    for (unsigned i = 0; i < 16; ++i) {
        unsigned char pixel[4] = {(unsigned char)(i * 16), 80, 160, 255};
        if (fwrite(pixel, 1, sizeof(pixel), file) != sizeof(pixel)) ok = 0;
    }
    if (fclose(file) != 0) ok = 0;
    return ok ? 0 : 1;
}
