#ifndef PORT_CAR_PARTS_H
#define PORT_CAR_PARTS_H
#include "game/car_asset.h"
#include "scene_matrix.h"

enum { CAR_PART_COUNT = 4 };
typedef struct CarPart {
    Vec3 position;
    SceneMat3 rotation;
} CarPart;

/* Body, rear wheels, front left, front right in game world coordinates.
 * No view matrix, globals or model banks. Input state remains unchanged. */
int BuildCarParts(const GameCarRuntime *car, const CarShape *shape,
                  s32 steering, CarPart parts[CAR_PART_COUNT]);
/* Body metadata is copied to all four parts; lamps stay on the body.
 * Explicit previous pose makes repeated builds independent of render history.
 * Failure preserves out. Sources/output must not overlap. */
RenderTransform CarPartTransform(const CarPart *part);
int BuildCarInstances(const GameCarRuntime *car, const GameCarRuntime *previous,
                      const CarShape *shape, const RenderMeshInstance *body,
                      u32 rearWheel, u32 frontWheel,
                      RenderMeshInstance out[CAR_PART_COUNT]);
#endif
