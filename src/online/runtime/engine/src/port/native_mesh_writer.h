#ifndef RAGE_NATIVE_MESH_WRITER_H
#define RAGE_NATIVE_MESH_WRITER_H

#include "common.h"
#include "game/vector.h"
#include "game/model_bank.h"
#include "game/terrain_bank.h"
#include "render/rmesh_cache.h"

typedef struct RageImportedTextureKey {
    uint16_t tpage;
    uint16_t clut;
    uint16_t windowWidthU;
    uint16_t windowWidthV;
    uint16_t windowOffsetU;
    uint16_t windowOffsetV;
    uint8_t hasWindow;
    uint8_t emissive;
    uint8_t terrainEnvironmentClut;
} RageImportedTextureKey;

typedef struct RageImportedMeshEntry {
    RageRuntimeCachedMesh cached;
    RageImportedTextureKey *materials;
    uint32_t materialCount;
    RenderAssetSource source;
    struct CarModelData *carSource;
} RageImportedMeshEntry;

/* Prepare a variant-indexed array of CAR_MODEL_VARIANT_COUNT sources.
 * Existing entries are immutable; failure releases only additions. */
int ImportPrepareCars(RageImportedMeshEntry *entries, uint32_t *count,
                      uint32_t capacity, struct CarModelData *const *models);
void ImportReleaseEntry(RageImportedMeshEntry *entry);
RageImportedMeshEntry *ImportFindMesh(RageImportedMeshEntry *entries, uint32_t count,
                                    uint32_t key, RenderAssetSet set, RenderAssetSource source);

typedef struct RageImportedFace {
    const SVec *vertices;
    const SVec *normals;
    uint16_t vertex[4];
    uint16_t normal[4];
    uint8_t uv[4][2];
    uint8_t color[3];
    RageImportedTextureKey texture;
    int8_t depthBias;
    uint8_t prim;
    uint8_t flags;
    uint8_t textured;
    uint8_t hasNormals;
} RageImportedFace;

typedef int (*RageImportedFaceVisitor)(uint32_t mesh,
                                       const RageImportedFace *face,
                                       void *context);

/* Bank must be resolved from validated immutable source storage. */
int ImportVisitModelBank(const NativeModelBank *bank,
                          RageImportedFaceVisitor visitor, void *context,
                          uint32_t *meshCount);

typedef int (*RageImportedMeshSource)(void *context, RageImportedFaceVisitor visitor,
                                      void *output, uint32_t *meshCount);

/* Output must be empty. Failure leaves it unchanged; success transfers mesh
 * bytes and materials to the caller. Sources are borrowed for both passes. */
int ImportBuildMeshEntry(const RenderMeshInstance *instance,
                         RageImportedMeshSource source, void *context,
                         RageImportedMeshEntry *destination);

/* Imports a validated bank into caller-owned native mesh bytes. No game slots
 * or renderer state; source storage need only survive this synchronous call. */
int ImportBuildBankMesh(const RenderMeshInstance *instance, const NativeModelBank *bank,
                        RageImportedMeshEntry *destination);

/* Models must borrow a validated immutable course bank for both passes. */
int ImportVisitCourseModels(const NativeCourseModel *models, s32 count,
                            RageImportedFaceVisitor visitor, void *context,
                            uint32_t *meshCount);
int ImportBuildCourseMesh(const RenderMeshInstance *instance, const CourseBank *bank,
                          RageImportedMeshEntry *destination);

/* Terrain sources must borrow validated immutable storage during both passes. */
int ImportVisitTerrainCells(const void *const *cells, s32 count, const SVec *vertices,
                            RageImportedFaceVisitor visitor, void *context,
                            uint32_t *meshCount);
int ImportBuildTerrainMesh(const RenderMeshInstance *instance, const TerrainBank *bank,
                           RageImportedMeshEntry *destination);

typedef struct RageImportedWrite {
    RageImportedMeshEntry *entry;
    uint8_t *offsets;
    uint8_t *vertexCursor;
    uint8_t *indexCursor;
    uint32_t currentMesh;
    uint32_t vertexCount;
    uint32_t indexCount;
    uint32_t meshLimit;
    uint32_t vertexLimit;
    uint32_t indexLimit;
} RageImportedWrite;

/* Private synchronous importer writer. Source arrays are borrowed during the
 * call; output storage/capacities belong to the sizing pass. */
int ImportTextureEqual(const RageImportedTextureKey *left,
                       const RageImportedTextureKey *right);
int ImportWriteFace(uint32_t mesh, const RageImportedFace *face, void *context);
/* Verify the second pass matches its sizing limits before filling trailing
 * empty submesh endpoints. Rejection leaves writer and output unchanged. */
int ImportWriteFinish(RageImportedWrite *write, uint32_t meshes);

#endif
