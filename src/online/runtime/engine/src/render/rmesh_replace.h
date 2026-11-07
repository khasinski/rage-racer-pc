#ifndef RAGE_RMESH_REPLACE_H
#define RAGE_RMESH_REPLACE_H
#include "rmesh.h"
/* Return malloc-owned bytes. Only the selected index range is replaced;
 * other parts, including animated wheels, retain their vertices and indices. */
void *RuntimeMeshReplace(const RageRuntimeMesh *base, uint32_t part,
    const RageRuntimeMesh *replacement, const uint32_t *materials,
    size_t materialCount, size_t *size);
#endif
