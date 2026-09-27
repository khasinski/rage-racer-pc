#include "client_frame.h"
#include "native_sky.h"
#include "environment_view.h"
#include <stdlib.h>
#include <limits.h>
#include <string.h>

static const RageRuntimeMesh *Resolve(void *context, const RenderMeshInstance *instance) {
    const RageImportedMeshEntry *entry = FindClientMesh(context, instance);
    return entry ? &entry->cached.mesh : NULL;
}

void FreeClientFrame(ClientFrame *frame) {
    if (!frame || !frame->references || --frame->references) return;
    ModernPreparedMeshesRelease(&frame->meshes);
    RenderWorldSnapshotRelease(&frame->scene);
    FreeClientRace(frame->race);
    free(frame);
}

ClientFrame *CaptureClientFrame(ClientRace *race, const RenderWorld *world, int page) {
    if (!race || !world || page < 0 || page > 1) return NULL;
    ClientFrame *frame = calloc(1, sizeof(*frame));
    if (!frame) return NULL;
    frame->references = 1;
    frame->race = RetainClientRace(race);
    if (!frame->race || !RenderWorldSnapshotCopy(&frame->scene, world)) goto fail;
    if (!ModernPreparedMeshesPrepare(&frame->meshes, &frame->scene.world, Resolve, race)) goto fail;
    for (u32 i = 0; i < frame->scene.world.instanceCount; ++i) {
        const RenderMeshInstance *instance = &frame->scene.world.instances[i];
        if (instance->pass == RAGE_RENDER_PASS_MAIN && !ModernPreparedMeshesLookup(&frame->meshes, instance)) goto fail;
    }
    if (!RetailSkyLayout(&frame->scene.world.camera.skyLayout, race->env.skyRowBase)) goto fail;
    frame->scene.world.camera.hasSkyLayout = 1;
    frame->scene.world.camera.skyAssetKey = race->primaryMesh.cached.assetKey;
    ApplyEnvironment(&frame->scene.world.camera, &race->env);
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

int DecodeFrameSky(const ClientFrame *frame, u8 *rgba, size_t size) {
    if (!frame || !frame->race || !frame->race->pixels ||
        !frame->scene.world.camera.hasSkyLayout || frame->page < 0 || frame->page > 1)
        return 0;
    const TextureImage image = {frame->race->pixels->pages[frame->page],
        RAGE_TRACK_VRAM_WIDTH * RAGE_TRACK_VRAM_HEIGHT,
        0, 0, RAGE_TRACK_VRAM_WIDTH, RAGE_TRACK_VRAM_HEIGHT, NULL};
    const TextureImage palette = {frame->palette, 16, 224, 486, 16, 1, &image};
    return DecodeSky(&palette, &frame->scene.world.camera.skyLayout, rgba, size);
}

ClientFrame *RetainClientFrame(ClientFrame *frame) {
    if (!frame || !frame->references || frame->references == UINT_MAX) return NULL;
    ++frame->references;
    return frame;
}
