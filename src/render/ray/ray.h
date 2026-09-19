#ifndef RAGE_RAY_H
#define RAGE_RAY_H

#include <stdint.h>

#include "render/render_world.h"

typedef struct Ray {
    Vec3 origin;
    Vec3 direction;
    float minDistance;
    float maxDistance;
} Ray;

typedef struct RayBounds {
    Vec3 min;
    Vec3 max;
} RayBounds;

typedef struct RayTriangle {
    Vec3 vertex[3];
    uint32_t material;
} RayTriangle;

typedef struct RayHit {
    float distance;
    float barycentricU;
    float barycentricV;
    Vec3 normal;
    uint32_t triangle;
    uint32_t material;
    uint32_t instance;
    uint32_t entity;
} RayHit;

enum {
    RAY_TRACE_CULL_BACKFACES = 1u << 0,
    RAY_TRACE_REVERSE_WINDING = 1u << 1,
};

int RayValid(const Ray *ray);
int RayIntersectBounds(const Ray *ray, const RayBounds *bounds,
                       float maximumDistance, float *nearDistance);
int RayIntersectTriangle(const Ray *ray, const RayTriangle *triangle,
                         uint32_t flags, RayHit *hit);

#endif
