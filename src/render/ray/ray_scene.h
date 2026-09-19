#ifndef RAGE_RAY_SCENE_H
#define RAGE_RAY_SCENE_H

#include "ray_bvh.h"
#include "render/render_world.h"

typedef struct RayInstance {
    const RayMesh *mesh;
    float localToWorld[3][3];
    float worldToLocal[3][3];
    Vec3 position;
    RayBounds bounds;
    float orientationSign;
    uint32_t entity;
    uint32_t flags;
} RayInstance;

typedef struct RayScene {
    RayInstance *instances;
    uint32_t *indices;
    RayBvhNode *nodes;
    uint32_t instanceCount;
    uint32_t nodeCount;
} RayScene;

enum {
    RAY_INSTANCE_CULL_BACKFACES = 1u << 0,
    RAY_INSTANCE_NO_SHADOW = 1u << 1,
};

/* Prepare one immutable mesh instance. Singular and non-finite transforms are
 * rejected because they cannot define a valid local-space ray. */
int RayInstancePrepare(RayInstance *out, const RayMesh *mesh,
                       const RenderTransform *transform, uint32_t entity,
                       uint32_t flags);
int RayInstanceTraceClosest(const RayInstance *instance, const Ray *worldRay,
                            RayHit *hit);
int RayInstanceTraceAny(const RayInstance *instance, const Ray *worldRay);

/* The scene owns a shallow copy of instances; referenced immutable meshes must
 * remain alive until release or replacement. Failed builds preserve the old
 * scene. */
int RaySceneBuild(RayScene *scene, const RayInstance *instances, size_t count);
void RaySceneRelease(RayScene *scene);
int RaySceneTraceClosest(const RayScene *scene, const Ray *ray, RayHit *hit);
int RaySceneTraceAny(const RayScene *scene, const Ray *ray);

#endif
