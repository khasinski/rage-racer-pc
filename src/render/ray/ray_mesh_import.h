#ifndef RAGE_RAY_MESH_IMPORT_H
#define RAGE_RAY_MESH_IMPORT_H

#include "ray_bvh.h"
#include "render/rmesh.h"

/* Build a ray mesh from one indexed RMESH submesh. The destination is replaced
 * only after the source range and every triangle have been validated. */
int RayMeshBuildRuntime(RayMesh *out, const RageRuntimeMesh *source,
                        uint32_t meshIndex);

#endif
