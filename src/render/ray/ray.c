#include "ray.h"

#include <float.h>
#include <math.h>
#include <stddef.h>

static Vec3 Subtract(Vec3 left, Vec3 right) {
    return (Vec3){left.x - right.x, left.y - right.y, left.z - right.z};
}

static Vec3 Cross(Vec3 left, Vec3 right) {
    return (Vec3){
        left.y * right.z - left.z * right.y,
        left.z * right.x - left.x * right.z,
        left.x * right.y - left.y * right.x,
    };
}

static float Dot(Vec3 left, Vec3 right) {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

static int VecFinite(Vec3 value) {
    return isfinite(value.x) && isfinite(value.y) && isfinite(value.z);
}

int RayValid(const Ray *ray) {
    float lengthSquared;

    if (ray == NULL || !VecFinite(ray->origin) ||
        !VecFinite(ray->direction) || !isfinite(ray->minDistance) ||
        !isfinite(ray->maxDistance) || ray->minDistance < 0.0f ||
        ray->maxDistance < ray->minDistance) {
        return 0;
    }
    lengthSquared = Dot(ray->direction, ray->direction);
    return isfinite(lengthSquared) && lengthSquared > FLT_MIN;
}

static int IntersectAxis(float origin, float direction, float lower,
                         float upper, float *nearDistance,
                         float *farDistance) {
    float first;
    float second;

    if (fabsf(direction) <= FLT_MIN) {
        return origin >= lower && origin <= upper;
    }
    first = (lower - origin) / direction;
    second = (upper - origin) / direction;
    if (first > second) {
        float temporary = first;
        first = second;
        second = temporary;
    }
    if (first > *nearDistance) *nearDistance = first;
    if (second < *farDistance) *farDistance = second;
    return *farDistance >= *nearDistance;
}

int RayIntersectBounds(const Ray *ray, const RayBounds *bounds,
                       float maximumDistance, float *nearDistance) {
    float nearValue;
    float farValue;

    if (!RayValid(ray) || bounds == NULL || !VecFinite(bounds->min) ||
        !VecFinite(bounds->max) || bounds->min.x > bounds->max.x ||
        bounds->min.y > bounds->max.y || bounds->min.z > bounds->max.z ||
        !isfinite(maximumDistance) || maximumDistance < ray->minDistance) {
        return 0;
    }
    nearValue = ray->minDistance;
    farValue = fminf(ray->maxDistance, maximumDistance);
    if (!IntersectAxis(ray->origin.x, ray->direction.x,
                       bounds->min.x, bounds->max.x,
                       &nearValue, &farValue) ||
        !IntersectAxis(ray->origin.y, ray->direction.y,
                       bounds->min.y, bounds->max.y,
                       &nearValue, &farValue) ||
        !IntersectAxis(ray->origin.z, ray->direction.z,
                       bounds->min.z, bounds->max.z,
                       &nearValue, &farValue)) {
        return 0;
    }
    if (nearDistance != NULL) *nearDistance = nearValue;
    return 1;
}

int RayIntersectTriangle(const Ray *ray, const RayTriangle *triangle,
                         uint32_t flags, RayHit *hit) {
    const float epsilon = 1.0e-7f;
    Vec3 edge1;
    Vec3 edge2;
    Vec3 crossDirection;
    Vec3 fromVertex;
    Vec3 crossOrigin;
    Vec3 normal;
    float determinant;
    float inverse;
    float u;
    float v;
    float distance;
    float normalLength;

    if (!RayValid(ray) || triangle == NULL || hit == NULL ||
        !VecFinite(triangle->vertex[0]) || !VecFinite(triangle->vertex[1]) ||
        !VecFinite(triangle->vertex[2])) {
        return 0;
    }
    edge1 = Subtract(triangle->vertex[1], triangle->vertex[0]);
    edge2 = Subtract(triangle->vertex[2], triangle->vertex[0]);
    crossDirection = Cross(ray->direction, edge2);
    determinant = Dot(edge1, crossDirection);
    if ((flags & RAY_TRACE_CULL_BACKFACES) != 0) {
        if (determinant <= epsilon) return 0;
    } else if (fabsf(determinant) <= epsilon) {
        return 0;
    }
    inverse = 1.0f / determinant;
    fromVertex = Subtract(ray->origin, triangle->vertex[0]);
    u = Dot(fromVertex, crossDirection) * inverse;
    if (u < 0.0f || u > 1.0f) return 0;
    crossOrigin = Cross(fromVertex, edge1);
    v = Dot(ray->direction, crossOrigin) * inverse;
    if (v < 0.0f || u + v > 1.0f) return 0;
    distance = Dot(edge2, crossOrigin) * inverse;
    if (distance < ray->minDistance || distance > ray->maxDistance) return 0;
    normal = Cross(edge1, edge2);
    normalLength = sqrtf(Dot(normal, normal));
    if (!(normalLength > epsilon) || !isfinite(normalLength)) return 0;
    hit->distance = distance;
    hit->barycentricU = u;
    hit->barycentricV = v;
    hit->normal = (Vec3){normal.x / normalLength,
                         normal.y / normalLength,
                         normal.z / normalLength};
    hit->material = triangle->material;
    return 1;
}
