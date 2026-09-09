#include "modern_sky_reprojection.h"

#include "modern_sky_geometry.h"

#include <math.h>
#include <stdint.h>

void ModernSkyReprojectPoint(const RageRenderCamera *source,
                             const RageRenderCamera *target,
                             float *x, float *y) {
    float determinant;
    float dx, dy, column, row;
    if (!source || !target || !x || !y) return;
    determinant = source->skyGridColumn.x * source->skyGridRow.y -
                  source->skyGridColumn.y * source->skyGridRow.x;
    if (fabsf(determinant) < 0.0001f) return;
    dx = *x - source->skyGridOrigin.x;
    dy = *y - source->skyGridOrigin.y;
    column = (dx * source->skyGridRow.y - dy * source->skyGridRow.x) /
             determinant;
    row = (source->skyGridColumn.x * dy - source->skyGridColumn.y * dx) /
          determinant;
    *x = target->skyGridOrigin.x + column * target->skyGridColumn.x +
         row * target->skyGridRow.x;
    *y = target->skyGridOrigin.y + column * target->skyGridColumn.y +
         row * target->skyGridRow.y;
}

int ModernSkyBuildSmoothQuad(const RageCapturePacket *packet,
                             const RageRenderCamera *source,
                             const RageRenderCamera *target,
                             float x[4], float y[4]) {
    int index, row, column;
    float originX, originY, cellX, cellY, sourceColumn;
    if (!packet || !target || !x || !y || packet->skyIndex == UINT16_MAX ||
        packet->skyIndex >= 96) return 0;
    index = packet->skyIndex;
    if (target->skyCloudRow == 0) {
        if (index >= 24) return 0;
        row = 0;
        originX = target->skyGridOrigin.z;
        originY = target->skyGridRow.z;
    } else {
        row = index / 24;
        originX = target->skyGridOrigin.x;
        originY = target->skyGridOrigin.y;
    }
    column = index % 24 - 8;
    sourceColumn = (float)column;
    if (source)
        sourceColumn += ModernSkySourceColumn(source->skyGridColumn.z,
                                              target->skyGridColumn.z);
    cellX = originX + sourceColumn * target->skyGridColumn.x -
            row * target->skyGridRow.x;
    cellY = originY + sourceColumn * target->skyGridColumn.y -
            row * target->skyGridRow.y;
    x[0] = cellX;
    y[0] = cellY;
    x[1] = cellX + target->skyGridColumn.x;
    y[1] = cellY + target->skyGridColumn.y;
    x[2] = cellX + target->skyGridRow.x;
    y[2] = cellY + target->skyGridRow.y;
    x[3] = x[1] + target->skyGridRow.x;
    y[3] = y[1] + target->skyGridRow.y;
    return 1;
}
