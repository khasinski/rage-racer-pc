#ifndef RAGE_RENDER_LOCAL_GEOMETRY_H
#define RAGE_RENDER_LOCAL_GEOMETRY_H

#include "render_instance_transform.h"
#include "render_mesh_build.h"

/* std140 ABI shared by main/mirror and shadow vertex shaders. Mode zero
 * bypasses local transforms; one preserves Euler order; two uses quaternion
 * matrix rows. Normals rotate without inverse scale, matching the CPU path. */
typedef struct RageNativeLocalUniform {
    float positionMode[4];
    float scaleFog[4];
    float rotation[3][4];
} RageNativeLocalUniform;
_Static_assert(sizeof(RageNativeLocalUniform) == 80, "local geometry uniform ABI");

static inline RageNativeLocalUniform RenderNativeLocalUniform(
    const RageNativeDrawSpan *span) {
    RageNativeLocalUniform out = {0};
    if (!span || !span->localGeometry) return out;
    RageRenderInstanceTransform b = RenderPrepareInstanceTransform(&span->localTransform);
    out.positionMode[0] = b.position.x; out.positionMode[1] = b.position.y;
    out.positionMode[2] = b.position.z; out.positionMode[3] = b.useMatrix ? 2 : 1;
    out.scaleFog[0] = b.scale.x; out.scaleFog[1] = b.scale.y; out.scaleFog[2] = b.scale.z;
    out.scaleFog[3] = !!(span->instanceFlags & RAGE_RENDER_INSTANCE_ENABLE_FOG);
    if (b.useMatrix) {
        for (unsigned row = 0; row < 3; ++row)
            for (unsigned col = 0; col < 3; ++col) out.rotation[row][col] = b.matrix[row][col];
    } else {
        out.rotation[0][0] = b.cx; out.rotation[0][1] = b.cy; out.rotation[0][2] = b.cz;
        out.rotation[1][0] = b.sx; out.rotation[1][1] = b.sy; out.rotation[1][2] = b.sz;
    }
    out.rotation[0][3] = span->depthDecal ? 2 : 0;
    return out;
}
#endif
