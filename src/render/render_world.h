#ifndef RAGE_RENDER_WORLD_H
#define RAGE_RENDER_WORLD_H

/*
 * Renderer-neutral scene data.  This is deliberately not a PS1 packet list:
 * mesh/material handles name imported assets, transforms are world-space
 * floats, and visibility is an ordinary scene property.  Classic and modern
 * renderers will consume this representation through separate adapters.
 */

#include <stdint.h>

typedef struct RageRenderVec3 {
    float x, y, z;
} RageRenderVec3;

/* A scene transform normally uses the friendly Euler form below.  Hierarchical
 * animated objects may instead provide a unit quaternion: it represents one
 * ordinary world-space rotation, rather than a renderer/backend matrix. */
typedef struct RageRenderQuaternion {
    float x, y, z, w;
} RageRenderQuaternion;

typedef struct RageRenderTransform {
    RageRenderVec3 position;
    RageRenderVec3 rotation;
    RageRenderVec3 scale;
    RageRenderQuaternion orientation;
    uint8_t hasOrientation;
} RageRenderTransform;

typedef struct RageRenderCamera {
    RageRenderTransform transform;
    float verticalFovDegrees;
    float nearPlane;
    float farPlane;
    /* Perspective depth fog is semantic scene data. `fogNear` starts the
     * blend and `fogFar` reaches the authored environment colour. */
    RageRenderVec3 fogColor;
    float fogNear;
    float fogFar;
} RageRenderCamera;

typedef enum RageRenderPass {
    RAGE_RENDER_PASS_MAIN = 0,
    RAGE_RENDER_PASS_MIRROR = 1,
} RageRenderPass;

/* Which imported asset collection owns `mesh`.  A numeric mesh id is only
 * meaningful inside one collection; making that explicit prevents a modern
 * backend from falling back to the currently-selected PS1 model bank. */
typedef enum RageRenderAssetSet {
    RAGE_RENDER_ASSET_MODEL_BANK = 0,
    RAGE_RENDER_ASSET_COURSE = 1,
    RAGE_RENDER_ASSET_TERRAIN = 2,
    /* Track packs contain two independent ordinary model banks.  They must
     * not share the generic `model` key: doing so made the runtime index pick
     * whichever duplicate happened to be listed first. */
    RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1 = 3,
    RAGE_RENDER_ASSET_TRACK_MODEL_BANK_2 = 4,
} RageRenderAssetSet;

typedef struct RageRenderMeshInstance {
    uint32_t entity;
    uint32_t mesh;
    RageRenderAssetSet assetSet;
    /* Stable game asset identity inside the asset set. It is never a pointer
     * into a loaded PS1 bank, so streaming/replay and native cache lookup are
     * deterministic. */
    uint32_t assetKey;
    uint32_t material;
    /* Renderer-neutral material variant selected by gameplay state. Course
     * and terrain use the two track sections; track car models use the
     * selected car asset whose race-load palette is resident. */
    uint8_t materialVariant;
    /* Some authored course faces add the low seven bits of g_AnimTimer to
     * their U coordinates before the PS1 texture window is applied.  The
     * imported mesh marks only those faces; the instance supplies the current
     * semantic texel offset without exposing a PS1 packet to the renderer. */
    uint8_t textureScrollU;
    /* Semantic ambient light sampled from the track light volume. The modern
     * backend combines it with its directional vehicle lighting. */
    RageRenderVec3 environmentLight;
    RageRenderTransform transform;
    RageRenderTransform previousTransform;
    uint32_t flags;
    RageRenderPass pass;
} RageRenderMeshInstance;

enum {
    /* Bounds are optional imported metadata. Do not cull an instance until
     * its producer has opted into the coordinate convention explicitly. */
    RAGE_RENDER_INSTANCE_ENABLE_FRUSTUM_CULL = 1u << 0,
    /* The original course data chooses fogged and un-fogged model dispatches
     * per object. Keep that authored choice in the scene, not the backend. */
    RAGE_RENDER_INSTANCE_ENABLE_FOG = 1u << 1,
    /* Native lighting is currently authored for vehicle normals. Course and
     * terrain retain their source texture brightness until their original
     * environment light matrices are represented in Render World. */
    RAGE_RENDER_INSTANCE_ENABLE_LIGHTING = 1u << 2,
    /* Terrain modes 0/1 choose the adjacent CLUT in environment mode 4. */
    RAGE_RENDER_INSTANCE_ENVIRONMENT_MODE_4 = 1u << 3,
};

typedef struct RageRenderWorld {
    uint64_t frame;
    RageRenderCamera camera;
    RageRenderCamera previousCamera;
    uint8_t hasCamera;
    RageRenderMeshInstance *instances;
    uint32_t instanceCapacity;
    uint32_t instanceCount;
    uint32_t overflowCount;
} RageRenderWorld;

void RageRenderWorldInit(RageRenderWorld *world,
                         RageRenderMeshInstance *instances,
                         uint32_t capacity);
void RageRenderWorldBeginFrame(RageRenderWorld *world, uint64_t frame);
void RageRenderWorldSetCamera(RageRenderWorld *world,
                              const RageRenderCamera *camera);
int RageRenderWorldSubmitMesh(RageRenderWorld *world,
                              const RageRenderMeshInstance *instance);
void RageRenderTerrainCellTransform(uint32_t grid_x, uint32_t grid_z,
                                    RageRenderTransform *transform);
/* Convert a PS1-space rotation into the conventional (+Y up, -Z forward)
 * scene basis used by imported meshes and Render World positions. */
void RageRenderConvertPsxMatrix(const float source[3][3], float out[3][3]);

#endif
