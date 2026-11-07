#include "rmesh_replace.h"
#include <stdlib.h>
#include <string.h>

static void Write32(uint8_t *p, uint32_t v) {
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24);
}

void *RuntimeMeshReplace(const RageRuntimeMesh *base, uint32_t part,
    const RageRuntimeMesh *replacement, const uint32_t *materials,
    size_t materialCount, size_t *size) {
    uint32_t first, removed, rfirst, added, i, cursor=0;
    uint64_t vertices, indices, total, vertexOffset, indexOffset;
    uint8_t *bytes;
    RageRuntimeMesh checkedBase, checkedReplacement;
    if (!size) return NULL;
    *size=0;
    if (!base || !replacement ||
        !RuntimeMeshOpen(&checkedBase,base->bytes,base->size) ||
        !RuntimeMeshOpen(&checkedReplacement,replacement->bytes,replacement->size)) return NULL;
    base=&checkedBase;
    replacement=&checkedReplacement;
    if (!base || !replacement || replacement->meshCount!=1 ||
        !RuntimeMeshRange(base,part,&first,&removed) ||
        !RuntimeMeshRange(replacement,0,&rfirst,&added) || !added) return NULL;
    vertices=(uint64_t)base->vertexCount+replacement->vertexCount;
    indices=(uint64_t)base->indexCount-removed+added;
    vertexOffset=24+((uint64_t)base->meshCount+1)*4;
    indexOffset=vertexOffset+vertices*40;
    total=indexOffset+indices*4;
    if (vertices>UINT32_MAX || indices>UINT32_MAX || total>SIZE_MAX) return NULL;
    bytes=malloc((size_t)total);
    if (!bytes) return NULL;
    memcpy(bytes,"RRMESH1\0",8);
    Write32(bytes+8,1); Write32(bytes+12,base->meshCount);
    Write32(bytes+16,(uint32_t)vertices); Write32(bytes+20,(uint32_t)indices);
    memcpy(bytes+vertexOffset,base->bytes+base->verticesOffset,(size_t)base->vertexCount*40);
    memcpy(bytes+vertexOffset+(size_t)base->vertexCount*40,
        replacement->bytes+replacement->verticesOffset,(size_t)replacement->vertexCount*40);
    for (i=0;i<replacement->vertexCount;i++) {
        RageRuntimeVertex v;
        uint32_t slot;
        if (!RuntimeMeshVertex(replacement,i,&v)) goto fail;
        slot=v.material&RAGE_RUNTIME_MATERIAL_INDEX_MASK;
        if (slot!=0xFFFFu && materials) {
            if (slot>=materialCount || materials[slot]>=0xFFFFu) goto fail;
            v.material=(v.material&~(uint32_t)RAGE_RUNTIME_MATERIAL_INDEX_MASK)|materials[slot];
        }
        Write32(bytes+vertexOffset+((size_t)base->vertexCount+i)*40+36,v.material);
    }
    for (i=0;i<base->meshCount;i++) {
        uint32_t begin, count, j;
        const RageRuntimeMesh *source=i==part?replacement:base;
        if (!RuntimeMeshRange(source,i==part?0:i,&begin,&count)) goto fail;
        Write32(bytes+24+(size_t)i*4,cursor);
        for (j=0;j<count;j++) {
            uint32_t index;
            if (!RuntimeMeshIndex(source,begin+j,&index)) goto fail;
            if (i==part) index+=base->vertexCount;
            Write32(bytes+indexOffset+(size_t)cursor++*4,index);
        }
    }
    Write32(bytes+24+(size_t)base->meshCount*4,cursor);
    *size=(size_t)total;
    return bytes;
fail:
    free(bytes);
    return NULL;
}
