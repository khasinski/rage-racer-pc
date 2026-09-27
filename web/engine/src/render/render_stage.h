#ifndef RAGE_RENDER_STAGE_H
#define RAGE_RENDER_STAGE_H

/*
 * Composing a scene from nothing, for looking at one thing on its own.
 *
 * The renderer takes a RenderWorld, and until now the only way to get one
 * was to capture a frame out of a running race. That makes a whole class of
 * question awkward to ask: how does this car look from behind, how do two
 * track pieces meet, does anything come apart at an angle the captured frames
 * never happened to contain. A stage answers those by placing named assets in
 * an empty world and pointing a camera at them.
 *
 * The camera orbits its target rather than being positioned directly, because
 * every question of this shape is "the same subject, seen from somewhere
 * else". None of this touches game state, so it runs without a disc, without
 * the game and without a window.
 */

#include "render_world.h"

/* Where the camera stands, in orbit around what it is looking at. Azimuth
 * turns around the subject and elevation climbs above it, both in degrees. */
typedef struct RenderStage {
    Vec3 target;
    float distance;
    float azimuthDegrees;
    float elevationDegrees;
    float rollDegrees;
    float verticalFovDegrees;
    float nearPlane;
    float farPlane;
} RenderStage;

/* One asset placed in the world. */
typedef struct RenderPose {
    RenderAssetSet assetSet;
    uint32_t assetKey;
    uint32_t mesh;
    uint8_t materialVariant;
    Vec3 position;
    Vec3 rotationDegrees;
    uint32_t flags;
    float lightInfluence;
    /* The scene carries a rotation either as the Euler triple above or as a
     * quaternion, and the renderer builds its transform from one or the
     * other through separate code. The game poses cars with a quaternion, so
     * a stage that only ever used the Euler form would be exercising the
     * branch the cars do not take. Set this to pose through the quaternion
     * the same angles describe. */
    uint8_t useQuaternion;
} RenderPose;

/* A stage that frames a car-sized subject, and a pose at the origin. */
void RenderStageDefaults(RenderStage *stage);
void RenderPoseDefaults(RenderPose *pose);

/* Place the camera on its orbit, looking at the target. */
void RenderStageCamera(const RenderStage *stage,
                       RenderCamera *camera);

/*
 * Fill `world` with the given poses seen from the given stage, using
 * `storage` for the instances. Returns the number placed, which is fewer than
 * `count` only when `capacity` runs out.
 */
uint32_t RenderStageCompose(RenderWorld *world,
                            RenderMeshInstance *storage,
                            uint32_t capacity,
                            const RenderStage *stage,
                            const RenderPose *poses, uint32_t count);

#endif
