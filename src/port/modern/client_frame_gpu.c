#include "modern_native_source.h"
#include <stdlib.h>
#include "../client_frame.h"

static void *Retain(void *context) { return RetainClientFrame(context); }
static void Release(void *context) { FreeClientFrame(context); }
static const RageRuntimeMesh *Mesh(void *context, const RenderMeshInstance *instance) {
    const ClientFrame *frame = context;
    const RageImportedMeshEntry *entry = FindClientMesh(frame->race, instance);
    return entry ? &entry->cached.mesh : NULL;
}
static int Material(void *context, const RenderMeshInstance *instance, u32 material,
                    RageRenderMaterial *definition, ModernAssetImage *image) {
    ModernAssetImage candidate = {malloc(256u * 256u * 4u),
                                 256u * 256u * 4u, 256, 256};
    if (!candidate.pixels || !DecodeFrameMaterial(context, instance, material,
                                                  candidate.pixels, candidate.size)) {
        free(candidate.pixels);
        return 0;
    }
    RenderMaterialDefault(definition);
    *image = candidate;
    return 1;
}
static void FreeImage(ModernAssetImage *image) {
    if (!image) return;
    free(image->pixels);
    *image = (ModernAssetImage){0};
}
static int Sky(void *context, ModernAssetImage *image) {
    ModernAssetImage candidate = {malloc(512u * 256u * 4u),
                                 512u * 256u * 4u, 512, 256};
    if (!candidate.pixels || !DecodeFrameSky(context, candidate.pixels, candidate.size)) {
        free(candidate.pixels);
        return 0;
    }
    *image = candidate;
    return 1;
}

void PrepareClientFrameGpu(ClientFrame *frame, float aspect) {
    if (!frame) return;
    uint64_t revision = UINT64_C(14695981039346656037);
    const u8 *bytes = (const u8 *)frame->palette;
    for (size_t i = 0; i < sizeof(frame->palette); ++i) {
        revision ^= bytes[i];
        revision *= UINT64_C(1099511628211);
    }
    revision ^= (u32)frame->page;
    const ModernNativeSource source = {frame, frame->race, revision,
                                      Retain, Release, Mesh, Material, Sky, FreeImage};
    ModernNativeGpuPrepareSource(&frame->scene.world, aspect, &source);
}
