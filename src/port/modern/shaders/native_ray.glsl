struct RayNode {
    vec4 minimum;
    vec4 maximum;
    uvec4 childAndRange;
};

struct RayTriangle {
    vec4 vertex0;
    vec4 vertex1;
    vec4 vertex2;
};

struct RayInstance {
    vec4 worldToLocal0;
    vec4 worldToLocal1;
    vec4 worldToLocal2;
    uvec4 meshAndFlags;
};

#ifndef RAY_NODE_BINDING
#define RAY_NODE_BINDING 2
#define RAY_TRIANGLE_BINDING 3
#define RAY_INDEX_BINDING 4
#define RAY_INSTANCE_BINDING 5
#endif

/* Leaves hold four triangles, so a tree over a few hundred thousand
 * triangles is under thirty levels deep; the traversals bail out when the
 * stack is exhausted rather than overrun it. A small stack keeps the
 * per-fragment scratch memory of two nested traversals affordable. */
#define RAY_STACK_SIZE 32
/* Nothing in a course is further than this from anything it can shade. */
#define RAY_FAR 60000.0

layout(set = 2, binding = RAY_NODE_BINDING, std430) readonly buffer RayNodes {
    RayNode rayNodes[];
};
layout(set = 2, binding = RAY_TRIANGLE_BINDING, std430) readonly buffer RayTriangles {
    RayTriangle rayTriangles[];
};
layout(set = 2, binding = RAY_INDEX_BINDING, std430) readonly buffer RayIndices {
    uint rayIndices[];
};
layout(set = 2, binding = RAY_INSTANCE_BINDING, std430) readonly buffer RayInstances {
    RayInstance rayInstances[];
};

/* Reciprocal direction with the near-axis-aligned components pushed away
 * from zero, so the slab test below never divides and never produces NaN. */
vec3 rayInverseDirection(vec3 direction) {
    vec3 safe = mix(direction, vec3(0.0000001), lessThan(abs(direction), vec3(0.0000001)));
    return 1.0 / safe;
}

/* Slab test against a node's bounds. `entry` receives the distance at which
 * the ray enters the box, which orders the children of a closest-hit walk. */
bool rayBoundsHit(vec3 origin, vec3 inverse, vec4 minimum, vec4 maximum,
                  float limit, out float entry) {
    vec3 first = (minimum.xyz - origin) * inverse;
    vec3 second = (maximum.xyz - origin) * inverse;
    vec3 near = min(first, second);
    vec3 far = max(first, second);
    float nearDistance = max(max(near.x, near.y), max(near.z, 0.02));
    float farDistance = min(min(far.x, far.y), min(far.z, limit));
    entry = nearDistance;
    return farDistance >= nearDistance;
}

bool rayTriangleHit(vec3 origin, vec3 direction, RayTriangle triangle,
                    float maximum) {
    vec3 edge1 = triangle.vertex1.xyz - triangle.vertex0.xyz;
    vec3 edge2 = triangle.vertex2.xyz - triangle.vertex0.xyz;
    vec3 crossDirection = cross(direction, edge2);
    float determinant = dot(edge1, crossDirection);
    if (abs(determinant) <= 0.0000001) return false;
    float inverse = 1.0 / determinant;
    vec3 fromVertex = origin - triangle.vertex0.xyz;
    float u = dot(fromVertex, crossDirection) * inverse;
    if (u < 0.0 || u > 1.0) return false;
    vec3 crossOrigin = cross(fromVertex, edge1);
    float v = dot(direction, crossOrigin) * inverse;
    if (v < 0.0 || u + v > 1.0) return false;
    float distance = dot(edge2, crossOrigin) * inverse;
    return distance >= 0.02 && distance <= maximum;
}

bool rayTriangleClosest(vec3 origin, vec3 direction, RayTriangle triangle,
                        float maximum, out float distance, out vec3 normal) {
    vec3 edge1 = triangle.vertex1.xyz - triangle.vertex0.xyz;
    vec3 edge2 = triangle.vertex2.xyz - triangle.vertex0.xyz;
    vec3 crossDirection = cross(direction, edge2);
    float determinant = dot(edge1, crossDirection);
    if (abs(determinant) <= 0.0000001) return false;
    float inverse = 1.0 / determinant;
    vec3 fromVertex = origin - triangle.vertex0.xyz;
    float u = dot(fromVertex, crossDirection) * inverse;
    if (u < 0.0 || u > 1.0) return false;
    vec3 crossOrigin = cross(fromVertex, edge1);
    float v = dot(direction, crossOrigin) * inverse;
    if (v < 0.0 || u + v > 1.0) return false;
    distance = dot(edge2, crossOrigin) * inverse;
    if (distance < 0.02 || distance > maximum) return false;
    normal = normalize(cross(edge1, edge2));
    return true;
}

/* Push the two children so that the one the ray enters first is visited
 * first: a closest-hit walk then shrinks its limit early and skips most of
 * the far subtree. Children that the ray misses are not pushed at all. */
void rayPushChildren(vec3 origin, vec3 inverse, float limit, RayNode node,
                     uint nodeCount, inout uint stack[RAY_STACK_SIZE],
                     inout uint stackCount) {
    uint left = node.childAndRange.x;
    uint right = node.childAndRange.y;
    float leftEntry = 0.0;
    float rightEntry = 0.0;
    bool hitLeft = left < nodeCount &&
        rayBoundsHit(origin, inverse, rayNodes[left].minimum,
                     rayNodes[left].maximum, limit, leftEntry);
    bool hitRight = right < nodeCount &&
        rayBoundsHit(origin, inverse, rayNodes[right].minimum,
                     rayNodes[right].maximum, limit, rightEntry);
    if (hitLeft && hitRight) {
        if (stackCount + 2 > RAY_STACK_SIZE) return;
        if (leftEntry <= rightEntry) {
            stack[stackCount++] = right;
            stack[stackCount++] = left;
        } else {
            stack[stackCount++] = left;
            stack[stackCount++] = right;
        }
    } else if (hitLeft || hitRight) {
        if (stackCount + 1 > RAY_STACK_SIZE) return;
        stack[stackCount++] = hitLeft ? left : right;
    }
}

bool rayMeshClosest(vec3 origin, vec3 direction, uint root, uint nodeCount,
                    float maximum, out float distance, out vec3 normal) {
    uint stack[RAY_STACK_SIZE];
    uint stackCount = 1;
    bool found = false;
    vec3 inverse = rayInverseDirection(direction);
    float entry;
    distance = maximum;
    stack[0] = root;
    if (root >= nodeCount ||
        !rayBoundsHit(origin, inverse, rayNodes[root].minimum,
                      rayNodes[root].maximum, distance, entry))
        return false;
    while (stackCount != 0) {
        uint nodeIndex = stack[--stackCount];
        RayNode node = rayNodes[nodeIndex];
        if (node.childAndRange.w != 0) {
            uint end = node.childAndRange.z + node.childAndRange.w;
            for (uint offset = node.childAndRange.z; offset < end; ++offset) {
                float candidateDistance;
                vec3 candidateNormal;
                uint triangleIndex = rayIndices[offset];
                if (rayTriangleClosest(origin, direction,
                        rayTriangles[triangleIndex], distance,
                        candidateDistance, candidateNormal)) {
                    distance = candidateDistance;
                    normal = candidateNormal;
                    found = true;
                }
            }
        } else {
            rayPushChildren(origin, inverse, distance, node, nodeCount,
                            stack, stackCount);
        }
    }
    return found;
}

bool rayMeshHit(vec3 origin, vec3 direction, uint root, uint nodeCount,
                float maximum) {
    uint stack[RAY_STACK_SIZE];
    uint stackCount = 1;
    vec3 inverse = rayInverseDirection(direction);
    float entry;
    stack[0] = root;
    while (stackCount != 0) {
        uint nodeIndex = stack[--stackCount];
        if (nodeIndex >= nodeCount) return false;
        RayNode node = rayNodes[nodeIndex];
        if (!rayBoundsHit(origin, inverse, node.minimum, node.maximum,
                          maximum, entry)) continue;
        if (node.childAndRange.w != 0) {
            uint end = node.childAndRange.z + node.childAndRange.w;
            for (uint offset = node.childAndRange.z; offset < end; ++offset) {
                uint triangleIndex = rayIndices[offset];
                if (rayTriangleHit(origin, direction,
                                   rayTriangles[triangleIndex], maximum))
                    return true;
            }
        } else {
            if (stackCount + 2 > RAY_STACK_SIZE) return false;
            stack[stackCount++] = node.childAndRange.y;
            stack[stackCount++] = node.childAndRange.x;
        }
    }
    return false;
}

float tracedVisibility(vec3 origin, vec3 direction, uint nodeCount,
                       uint instanceCount) {
    uint stack[RAY_STACK_SIZE];
    uint stackCount = 1;
    vec3 inverse = rayInverseDirection(direction);
    float entry;
    stack[0] = 0;
    while (stackCount != 0) {
        uint nodeIndex = stack[--stackCount];
        if (nodeIndex >= nodeCount) return 1.0;
        RayNode node = rayNodes[nodeIndex];
        if (!rayBoundsHit(origin, inverse, node.minimum, node.maximum,
                          RAY_FAR, entry)) continue;
        if (node.childAndRange.w != 0) {
            uint end = node.childAndRange.z + node.childAndRange.w;
            for (uint offset = node.childAndRange.z; offset < end; ++offset) {
                uint instanceIndex = rayIndices[offset];
                if (instanceIndex >= instanceCount) return 1.0;
                RayInstance instance = rayInstances[instanceIndex];
                if ((instance.meshAndFlags.z & 2u) != 0u) continue;
                vec4 point = vec4(origin, 1.0);
                vec4 vector = vec4(direction, 0.0);
                vec3 localOrigin = vec3(dot(instance.worldToLocal0, point),
                                        dot(instance.worldToLocal1, point),
                                        dot(instance.worldToLocal2, point));
                vec3 localDirection = vec3(dot(instance.worldToLocal0, vector),
                                           dot(instance.worldToLocal1, vector),
                                           dot(instance.worldToLocal2, vector));
                if (rayMeshHit(localOrigin, localDirection,
                               instance.meshAndFlags.x, nodeCount, RAY_FAR))
                    return 0.0;
            }
        } else {
            if (stackCount + 2 > RAY_STACK_SIZE) return 1.0;
            stack[stackCount++] = node.childAndRange.y;
            stack[stackCount++] = node.childAndRange.x;
        }
    }
    return 1.0;
}

bool tracedClosest(vec3 origin, vec3 direction, uint nodeCount,
                   uint instanceCount, float maximum, out float distance,
                   out vec3 normal) {
    uint stack[RAY_STACK_SIZE];
    uint stackCount = 1;
    bool found = false;
    vec3 inverse = rayInverseDirection(direction);
    float entry;
    distance = maximum;
    stack[0] = 0;
    if (nodeCount == 0 ||
        !rayBoundsHit(origin, inverse, rayNodes[0].minimum,
                      rayNodes[0].maximum, distance, entry))
        return false;
    while (stackCount != 0) {
        uint nodeIndex = stack[--stackCount];
        RayNode node = rayNodes[nodeIndex];
        if (node.childAndRange.w != 0) {
            uint end = node.childAndRange.z + node.childAndRange.w;
            for (uint offset = node.childAndRange.z; offset < end; ++offset) {
                uint instanceIndex = rayIndices[offset];
                if (instanceIndex >= instanceCount) return found;
                RayInstance instance = rayInstances[instanceIndex];
                vec4 point = vec4(origin, 1.0);
                vec4 vector = vec4(direction, 0.0);
                vec3 localOrigin = vec3(dot(instance.worldToLocal0, point),
                                        dot(instance.worldToLocal1, point),
                                        dot(instance.worldToLocal2, point));
                vec3 localDirection = vec3(dot(instance.worldToLocal0, vector),
                                           dot(instance.worldToLocal1, vector),
                                           dot(instance.worldToLocal2, vector));
                float candidateDistance;
                vec3 localNormal;
                if (rayMeshClosest(localOrigin, localDirection,
                        instance.meshAndFlags.x, nodeCount, distance,
                        candidateDistance, localNormal)) {
                    distance = candidateDistance;
                    normal = normalize(vec3(
                        dot(vec3(instance.worldToLocal0.x,
                                 instance.worldToLocal1.x,
                                 instance.worldToLocal2.x), localNormal),
                        dot(vec3(instance.worldToLocal0.y,
                                 instance.worldToLocal1.y,
                                 instance.worldToLocal2.y), localNormal),
                        dot(vec3(instance.worldToLocal0.z,
                                 instance.worldToLocal1.z,
                                 instance.worldToLocal2.z), localNormal)));
                    if (dot(normal, direction) > 0.0) normal = -normal;
                    found = true;
                }
            }
        } else {
            rayPushChildren(origin, inverse, distance, node, nodeCount,
                            stack, stackCount);
        }
    }
    return found;
}
