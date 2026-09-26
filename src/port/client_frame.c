#include "client_frame.h"
#include <stdlib.h>
#include <string.h>

static const RageRuntimeMesh *Resolve(void *context, const RenderMeshInstance *instance) {
    const RageImportedMeshEntry *entry = FindClientMesh(context, instance);
    return entry ? &entry->cached.mesh : NULL;
}

void FreeClientFrame(ClientFrame *frame) {
    if (!frame) return;
    ModernPreparedMeshesRelease(&frame->meshes);
    RenderWorldSnapshotRelease(&frame->scene);
    FreeClientRace(frame->race);
    free(frame);
}

ClientFrame *CaptureClientFrame(ClientRace *race, const RenderWorld *world, int page) {
    if (!race || !world || page < 0 || page > 1) return NULL;
    ClientFrame *frame = calloc(1, sizeof(*frame));
    if (!frame) return NULL;
    frame->race = RetainClientRace(race);
    if (!frame->race || !RenderWorldSnapshotCopy(&frame->scene, world)) goto fail;
    for (u32 i = 0; i < frame->scene.world.instanceCount; ++i) {
        const RenderMeshInstance *instance = &frame->scene.world.instances[i];
        if (instance->pass == RAGE_RENDER_PASS_MAIN && !FindClientMesh(race, instance)) goto fail;
    }
    if (!ModernPreparedMeshesPrepare(&frame->meshes, &frame->scene.world, Resolve, race)) goto fail;
    memcpy(frame->palette, race->env.clut, sizeof(frame->palette));
    frame->page = page;
    return frame;
fail:
    FreeClientFrame(frame);
    return NULL;
}

const RageRuntimeMesh *ClientFrameMeshLookup(void *context, const RenderMeshInstance *instance) {
    const ClientFrame *frame = context;
    return frame ? ModernPreparedMeshesLookup(&frame->meshes, instance) : NULL;
}

int DecodeFrameMaterial(const ClientFrame *frame, const RenderMeshInstance *instance,
                         u32 material, u8 *rgba, size_t size) {
    return frame && DecodeClientMaterial(frame->race, instance, material, frame->page,
                                         frame->palette, rgba, size);
}
