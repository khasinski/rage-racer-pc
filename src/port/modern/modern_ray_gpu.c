#include "modern_ray_gpu.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "render/ray/ray_draws.h"
#include "render/ray/ray_gpu.h"

typedef struct RayBuffers {
    SDL_GPUBuffer *nodes;
    SDL_GPUBuffer *triangles;
    SDL_GPUBuffer *indices;
    uint32_t nodeBytes;
    uint32_t triangleBytes;
    uint32_t indexBytes;
} RayBuffers;

static SDL_GPUDevice *s_device;
static RayBuffers s_buffers;
static SDL_GPUTransferBuffer *s_pendingTransfer;
static uint32_t s_nodeCount;

static void ReleaseBuffers(RayBuffers *buffers) {
    if (s_device != NULL) {
        if (buffers->nodes != NULL)
            SDL_ReleaseGPUBuffer(s_device, buffers->nodes);
        if (buffers->triangles != NULL)
            SDL_ReleaseGPUBuffer(s_device, buffers->triangles);
        if (buffers->indices != NULL)
            SDL_ReleaseGPUBuffer(s_device, buffers->indices);
    }
    *buffers = (RayBuffers){0};
}

static int EnsureBuffers(const RayGpuLayout *layout) {
    RayBuffers next = {0};
    SDL_GPUBufferCreateInfo info = {
        .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
    };
    if (layout->nodeBytes > UINT32_MAX ||
        layout->triangleBytes > UINT32_MAX || layout->indexBytes > UINT32_MAX)
        return 0;
    if (s_buffers.nodes != NULL && s_buffers.nodeBytes >= layout->nodeBytes &&
        s_buffers.triangleBytes >= layout->triangleBytes &&
        s_buffers.indexBytes >= layout->indexBytes) return 1;
    next.nodeBytes = (uint32_t)layout->nodeBytes;
    next.triangleBytes = (uint32_t)layout->triangleBytes;
    next.indexBytes = (uint32_t)layout->indexBytes;
    info.size = next.nodeBytes;
    next.nodes = SDL_CreateGPUBuffer(s_device, &info);
    info.size = next.triangleBytes;
    next.triangles = SDL_CreateGPUBuffer(s_device, &info);
    info.size = next.indexBytes;
    next.indices = SDL_CreateGPUBuffer(s_device, &info);
    if (next.nodes == NULL || next.triangles == NULL || next.indices == NULL) {
        ReleaseBuffers(&next);
        return 0;
    }
    ReleaseBuffers(&s_buffers);
    s_buffers = next;
    return 1;
}

int ModernRayGpuInit(SDL_GPUDevice *device) {
    if (device == NULL) return 0;
    s_device = device;
    return 1;
}

void ModernRayGpuSubmitted(void) {
    if (s_device != NULL && s_pendingTransfer != NULL)
        SDL_ReleaseGPUTransferBuffer(s_device, s_pendingTransfer);
    s_pendingTransfer = NULL;
}

void ModernRayGpuShutdown(void) {
    ModernRayGpuSubmitted();
    ReleaseBuffers(&s_buffers);
    s_device = NULL;
    s_nodeCount = 0;
}

int ModernRayGpuPrepare(SDL_GPUCommandBuffer *command,
                        const RageNativeGpuVertex *vertices,
                        uint32_t vertexCount,
                        const RageNativeDrawSpan *spans,
                        uint32_t spanCount, RayDrawInclude include,
                        void *context) {
    RayMesh mesh = {0};
    RayGpuLayout layout;
    SDL_GPUTransferBufferCreateInfo transferInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
    };
    SDL_GPUTransferBuffer *transfer = NULL;
    SDL_GPUCopyPass *copy = NULL;
    uint8_t *mapped;
    uint64_t total;
    int result = 0;

    s_nodeCount = 0;
    if (s_device == NULL || command == NULL || s_pendingTransfer != NULL ||
        !RayMeshBuildDraws(&mesh, vertices, vertexCount, spans, spanCount,
                           include, context) ||
        !RayGpuLayoutForMesh(&mesh, &layout) || !EnsureBuffers(&layout))
        goto done;
    total = layout.nodeBytes + layout.triangleBytes + layout.indexBytes;
    if (total > UINT32_MAX) goto done;
    transferInfo.size = (uint32_t)total;
    transfer = SDL_CreateGPUTransferBuffer(s_device, &transferInfo);
    mapped = transfer != NULL
        ? SDL_MapGPUTransferBuffer(s_device, transfer, false) : NULL;
    if (mapped == NULL) goto done;
    if (!RayGpuPackMesh(&mesh,
                        (RayGpuNode *)mapped, mesh.nodeCount,
                        (RayGpuTriangle *)(mapped + layout.nodeBytes),
                        mesh.triangleCount,
                        (uint32_t *)(mapped + layout.nodeBytes +
                                     layout.triangleBytes),
                        mesh.triangleCount)) {
        SDL_UnmapGPUTransferBuffer(s_device, transfer);
        goto done;
    }
    SDL_UnmapGPUTransferBuffer(s_device, transfer);
    copy = SDL_BeginGPUCopyPass(command);
    if (copy == NULL) goto done;
    {
        SDL_GPUTransferBufferLocation source = {.transfer_buffer = transfer};
        SDL_GPUBufferRegion target = {
            .buffer = s_buffers.nodes, .size = (uint32_t)layout.nodeBytes};
        SDL_UploadToGPUBuffer(copy, &source, &target, true);
        source.offset = (uint32_t)layout.nodeBytes;
        target.buffer = s_buffers.triangles;
        target.size = (uint32_t)layout.triangleBytes;
        SDL_UploadToGPUBuffer(copy, &source, &target, true);
        source.offset = (uint32_t)(layout.nodeBytes + layout.triangleBytes);
        target.buffer = s_buffers.indices;
        target.size = (uint32_t)layout.indexBytes;
        SDL_UploadToGPUBuffer(copy, &source, &target, true);
    }
    SDL_EndGPUCopyPass(copy);
    copy = NULL;
    s_pendingTransfer = transfer;
    transfer = NULL;
    s_nodeCount = mesh.nodeCount;
    result = 1;
done:
    if (copy != NULL) SDL_EndGPUCopyPass(copy);
    if (transfer != NULL) SDL_ReleaseGPUTransferBuffer(s_device, transfer);
    RayMeshRelease(&mesh);
    return result;
}

void ModernRayGpuBind(SDL_GPURenderPass *pass) {
    SDL_GPUBuffer *buffers[3];
    if (pass == NULL || s_nodeCount == 0) return;
    buffers[0] = s_buffers.nodes;
    buffers[1] = s_buffers.triangles;
    buffers[2] = s_buffers.indices;
    SDL_BindGPUFragmentStorageBuffers(pass, 0, buffers, 3);
}

uint32_t ModernRayGpuNodeCount(void) { return s_nodeCount; }
