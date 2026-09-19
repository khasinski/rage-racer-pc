#include "modern_ray_gpu.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "render/ray/ray_gpu.h"
#include "render/ray/ray_mesh_import.h"
#include "render/ray/ray_world.h"

typedef struct RayBuffers {
    SDL_GPUBuffer *nodes, *triangles, *indices, *instances;
    uint32_t nodeBytes, triangleBytes, indexBytes, instanceBytes;
} RayBuffers;

typedef struct CachedMesh {
    const RageRuntimeMesh *source;
    uint32_t submesh;
    RayMesh mesh;
    struct CachedMesh *next;
} CachedMesh;

typedef struct MeshLookup {
    RageRenderMeshLookup resolve;
    void *context;
} MeshLookup;

static SDL_GPUDevice *s_device;
static RayBuffers s_buffers;
static SDL_GPUTransferBuffer *s_pendingTransfer;
static CachedMesh *s_meshes;
static const RayMesh **s_staticMeshes;
static uint32_t s_staticMeshCount, s_staticMeshCapacity;
static uint32_t s_staticTlasNodes, s_staticInstances;
static RayGpuSceneLayout s_staticLayout;
static int s_staticValid;
static uint64_t s_assetGeneration = UINT64_MAX;
static uint32_t s_nodeCount, s_triangleCount, s_instanceCount;
static uint32_t s_uploadBytes;
static int s_reusedStatic;
static uint64_t s_buildNanoseconds;

static void ReleaseBuffers(RayBuffers *buffers) {
    if (s_device != NULL) {
        if (buffers->nodes != NULL)
            SDL_ReleaseGPUBuffer(s_device, buffers->nodes);
        if (buffers->triangles != NULL)
            SDL_ReleaseGPUBuffer(s_device, buffers->triangles);
        if (buffers->indices != NULL)
            SDL_ReleaseGPUBuffer(s_device, buffers->indices);
        if (buffers->instances != NULL)
            SDL_ReleaseGPUBuffer(s_device, buffers->instances);
    }
    *buffers = (RayBuffers){0};
}

static void ReleaseMeshes(void) {
    while (s_meshes != NULL) {
        CachedMesh *entry = s_meshes;
        s_meshes = entry->next;
        RayMeshRelease(&entry->mesh);
        free(entry);
    }
    free(s_staticMeshes);
    s_staticMeshes = NULL;
    s_staticMeshCount = s_staticMeshCapacity = 0;
    s_staticValid = 0;
}

static int EnsureBuffers(const RayGpuSceneLayout *layout) {
    RayBuffers next = {0};
    SDL_GPUBufferCreateInfo info = {
        .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
    };
    if (layout->nodeBytes > UINT32_MAX ||
        layout->triangleBytes > UINT32_MAX || layout->indexBytes > UINT32_MAX ||
        layout->instanceBytes > UINT32_MAX)
        return 0;
    if (s_buffers.nodes != NULL && s_buffers.nodeBytes >= layout->nodeBytes &&
        s_buffers.triangleBytes >= layout->triangleBytes &&
        s_buffers.indexBytes >= layout->indexBytes &&
        s_buffers.instanceBytes >= layout->instanceBytes)
        return 1;
    next.nodeBytes = (uint32_t)layout->nodeBytes;
    next.triangleBytes = (uint32_t)layout->triangleBytes;
    next.indexBytes = (uint32_t)layout->indexBytes;
    next.instanceBytes = (uint32_t)layout->instanceBytes;
#define CREATE_BUFFER(member, bytes) do {                                    \
    info.size = next.bytes;                                                  \
    next.member = SDL_CreateGPUBuffer(s_device, &info);                      \
} while (0)
    CREATE_BUFFER(nodes, nodeBytes);
    CREATE_BUFFER(triangles, triangleBytes);
    CREATE_BUFFER(indices, indexBytes);
    CREATE_BUFFER(instances, instanceBytes);
#undef CREATE_BUFFER
    if (next.nodes == NULL || next.triangles == NULL || next.indices == NULL ||
        next.instances == NULL) {
        ReleaseBuffers(&next);
        return 0;
    }
    ReleaseBuffers(&s_buffers);
    s_buffers = next;
    s_staticValid = 0;
    return 1;
}

static uint32_t UniqueMeshCount(const RayScene *scene) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < scene->instanceCount; ++i) {
        int first = 1;
        for (uint32_t prior = 0; prior < i; ++prior) {
            if (scene->instances[prior].mesh == scene->instances[i].mesh) {
                first = 0;
                break;
            }
        }
        if (first) ++count;
    }
    return count;
}

static int StaticSceneMatches(const RayScene *scene,
                              const RayGpuSceneLayout *layout) {
    uint32_t unique = 0;
    if (!s_staticValid || s_staticTlasNodes != scene->nodeCount ||
        s_staticInstances != scene->instanceCount ||
        memcmp(&s_staticLayout, layout, sizeof(*layout)) != 0)
        return 0;
    for (uint32_t i = 0; i < scene->instanceCount; ++i) {
        int first = 1;
        for (uint32_t prior = 0; prior < i; ++prior) {
            if (scene->instances[prior].mesh == scene->instances[i].mesh) {
                first = 0;
                break;
            }
        }
        if (!first) continue;
        if (unique >= s_staticMeshCount ||
            s_staticMeshes[unique] != scene->instances[i].mesh)
            return 0;
        ++unique;
    }
    return unique == s_staticMeshCount;
}

static void RememberStaticScene(const RayScene *scene,
                                const RayGpuSceneLayout *layout) {
    uint32_t count = UniqueMeshCount(scene);
    uint32_t unique = 0;
    if (count > s_staticMeshCapacity) {
        const RayMesh **meshes = realloc(
            s_staticMeshes, (size_t)count * sizeof(*s_staticMeshes));
        if (meshes == NULL) {
            s_staticValid = 0;
            return;
        }
        s_staticMeshes = meshes;
        s_staticMeshCapacity = count;
    }
    for (uint32_t i = 0; i < scene->instanceCount; ++i) {
        int first = 1;
        for (uint32_t prior = 0; prior < i; ++prior) {
            if (scene->instances[prior].mesh == scene->instances[i].mesh) {
                first = 0;
                break;
            }
        }
        if (first) s_staticMeshes[unique++] = scene->instances[i].mesh;
    }
    s_staticMeshCount = count;
    s_staticTlasNodes = scene->nodeCount;
    s_staticInstances = scene->instanceCount;
    s_staticLayout = *layout;
    s_staticValid = 1;
}

static const RayMesh *LookupRayMesh(void *opaque,
                                    const RenderMeshInstance *instance) {
    MeshLookup *lookup = opaque;
    const RageRuntimeMesh *source = lookup->resolve(lookup->context, instance);
    CachedMesh *entry;
    if (source == NULL) return NULL;
    for (entry = s_meshes; entry != NULL; entry = entry->next) {
        if (entry->source == source && entry->submesh == instance->mesh)
            return &entry->mesh;
    }
    entry = calloc(1, sizeof(*entry));
    if (entry == NULL ||
        !RayMeshBuildRuntime(&entry->mesh, source, instance->mesh)) {
        free(entry);
        return NULL;
    }
    entry->source = source;
    entry->submesh = instance->mesh;
    entry->next = s_meshes;
    s_meshes = entry;
    return &entry->mesh;
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
    ReleaseMeshes();
    s_device = NULL;
    s_assetGeneration = UINT64_MAX;
    s_nodeCount = s_triangleCount = s_instanceCount = 0;
    s_uploadBytes = 0;
    s_reusedStatic = 0;
    s_buildNanoseconds = 0;
}

int ModernRayGpuPrepare(SDL_GPUCommandBuffer *command,
                        const RenderWorld *world,
                        RageRenderMeshLookup resolve, void *context,
                        uint64_t assetGeneration) {
    RayScene scene = {0};
    RayGpuSceneLayout layout;
    MeshLookup lookup = {resolve, context};
    SDL_GPUTransferBufferCreateInfo transferInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
    };
    SDL_GPUTransferBuffer *transfer = NULL;
    SDL_GPUCopyPass *copy = NULL;
    uint8_t *mapped;
    uint64_t total;
    int dynamic;
    int result = 0;
    uint64_t started = SDL_GetTicksNS();

    s_nodeCount = s_triangleCount = s_instanceCount = 0;
    s_uploadBytes = 0;
    s_reusedStatic = 0;
    if (s_device == NULL || command == NULL || world == NULL ||
        resolve == NULL || s_pendingTransfer != NULL)
        goto done;
    if (s_assetGeneration != assetGeneration) {
        ReleaseMeshes();
        s_assetGeneration = assetGeneration;
    }
    if (!RaySceneBuildWorld(&scene, world, RAGE_RENDER_PASS_MAIN,
                            LookupRayMesh, &lookup) ||
        !RayGpuLayoutForScene(&scene, &layout) || !EnsureBuffers(&layout))
        goto done;
    dynamic = StaticSceneMatches(&scene, &layout);
    total = dynamic
        ? (uint64_t)scene.nodeCount * sizeof(RayGpuNode) +
              (uint64_t)scene.instanceCount * sizeof(uint32_t) +
              layout.instanceBytes
        : layout.nodeBytes + layout.triangleBytes + layout.indexBytes +
              layout.instanceBytes;
    if (total > UINT32_MAX) goto done;
    transferInfo.size = (uint32_t)total;
    transfer = SDL_CreateGPUTransferBuffer(s_device, &transferInfo);
    mapped = transfer != NULL
        ? SDL_MapGPUTransferBuffer(s_device, transfer, false) : NULL;
    if (mapped == NULL) goto done;
    if (dynamic ? !RayGpuPackSceneDynamic(
            &scene, &layout, (RayGpuNode *)mapped, scene.nodeCount,
            (uint32_t *)(mapped + (size_t)scene.nodeCount * sizeof(RayGpuNode)),
            scene.instanceCount,
            (RayGpuInstance *)(mapped +
                (size_t)scene.nodeCount * sizeof(RayGpuNode) +
                (size_t)scene.instanceCount * sizeof(uint32_t)),
            layout.instanceCount)
        : !RayGpuPackScene(
            &scene, &layout, (RayGpuNode *)mapped, layout.nodeCount,
            (RayGpuTriangle *)(mapped + layout.nodeBytes), layout.triangleCount,
            (uint32_t *)(mapped + layout.nodeBytes + layout.triangleBytes),
            layout.indexCount,
            (RayGpuInstance *)(mapped + layout.nodeBytes + layout.triangleBytes +
                               layout.indexBytes), layout.instanceCount)) {
        SDL_UnmapGPUTransferBuffer(s_device, transfer);
        goto done;
    }
    SDL_UnmapGPUTransferBuffer(s_device, transfer);
    copy = SDL_BeginGPUCopyPass(command);
    if (copy == NULL) goto done;
    {
        SDL_GPUTransferBufferLocation source = {.transfer_buffer = transfer};
        SDL_GPUBufferRegion target = {0};
        if (dynamic) {
            target.buffer = s_buffers.nodes;
            target.size = scene.nodeCount * sizeof(RayGpuNode);
            /* Partial updates must retain the BLAS suffix already stored in
             * this buffer. Cycling selects fresh backing storage whose
             * untouched range is undefined, which made static scenery flash. */
            SDL_UploadToGPUBuffer(copy, &source, &target, false);
            source.offset += target.size;
            target.buffer = s_buffers.indices;
            target.size = scene.instanceCount * sizeof(uint32_t);
            SDL_UploadToGPUBuffer(copy, &source, &target, false);
            source.offset += target.size;
            target.buffer = s_buffers.instances;
            target.size = (uint32_t)layout.instanceBytes;
            SDL_UploadToGPUBuffer(copy, &source, &target, false);
        } else {
#define UPLOAD_BUFFER(member, bytes) do {                                    \
    target.buffer = s_buffers.member;                                        \
    target.size = (uint32_t)layout.bytes;                                    \
    SDL_UploadToGPUBuffer(copy, &source, &target, true);                      \
    source.offset += target.size;                                            \
} while (0)
        UPLOAD_BUFFER(nodes, nodeBytes);
        UPLOAD_BUFFER(triangles, triangleBytes);
        UPLOAD_BUFFER(indices, indexBytes);
        UPLOAD_BUFFER(instances, instanceBytes);
#undef UPLOAD_BUFFER
        }
    }
    SDL_EndGPUCopyPass(copy);
    copy = NULL;
    s_pendingTransfer = transfer;
    transfer = NULL;
    s_nodeCount = layout.nodeCount;
    s_triangleCount = layout.triangleCount;
    s_instanceCount = layout.instanceCount;
    s_uploadBytes = (uint32_t)total;
    s_reusedStatic = dynamic;
    if (!dynamic) RememberStaticScene(&scene, &layout);
    result = 1;
done:
    if (copy != NULL) SDL_EndGPUCopyPass(copy);
    if (transfer != NULL) SDL_ReleaseGPUTransferBuffer(s_device, transfer);
    RaySceneRelease(&scene);
    s_buildNanoseconds = SDL_GetTicksNS() - started;
    return result;
}

void ModernRayGpuBind(SDL_GPURenderPass *pass) {
    SDL_GPUBuffer *buffers[4];
    if (pass == NULL || s_nodeCount == 0) return;
    buffers[0] = s_buffers.nodes;
    buffers[1] = s_buffers.triangles;
    buffers[2] = s_buffers.indices;
    buffers[3] = s_buffers.instances;
    SDL_BindGPUFragmentStorageBuffers(pass, 0, buffers, 4);
}

uint32_t ModernRayGpuNodeCount(void) { return s_nodeCount; }
uint32_t ModernRayGpuTriangleCount(void) { return s_triangleCount; }
uint32_t ModernRayGpuInstanceCount(void) { return s_instanceCount; }
uint32_t ModernRayGpuUploadBytes(void) { return s_uploadBytes; }
int ModernRayGpuReusedStatic(void) { return s_reusedStatic; }
uint64_t ModernRayGpuBuildNanoseconds(void) { return s_buildNanoseconds; }
