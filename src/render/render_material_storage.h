#ifndef RAGE_RENDER_MATERIAL_STORAGE_H
#define RAGE_RENDER_MATERIAL_STORAGE_H
#include "render_material.h"
#include <string.h>
/* Caller-owned path bytes. Keep this storage at a stable address while using
 * the material view; copying the storage does not rebind existing views. */
typedef struct RageRenderMaterialStorage {
    char baseColorTexture[1024];
    char paintMask[1024];
} RageRenderMaterialStorage;
static inline int RenderMaterialStorePaths(RageRenderMaterial *material,RageRenderMaterialStorage *storage) {
    if(!material||!storage)return 0;
    RageRenderMaterialPath base=material->baseColorTexture,paint=material->paintMask;
    if(base.length>=sizeof(storage->baseColorTexture)||paint.length>=sizeof(storage->paintMask)||
       (base.length&&!base.text)||(paint.length&&!paint.text))return 0;
    RageRenderMaterialStorage copy;
    if(base.length)memcpy(copy.baseColorTexture,base.text,base.length);
    copy.baseColorTexture[base.length]=0;
    if(paint.length)memcpy(copy.paintMask,paint.text,paint.length);
    copy.paintMask[paint.length]=0;
    memcpy(storage->baseColorTexture,copy.baseColorTexture,base.length+1);
    memcpy(storage->paintMask,copy.paintMask,paint.length+1);
    material->baseColorTexture.text=base.length?storage->baseColorTexture:NULL;
    material->paintMask.text=paint.length?storage->paintMask:NULL;
    return 1;
}
#endif
