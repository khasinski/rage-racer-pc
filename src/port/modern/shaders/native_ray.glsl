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

bool rayBoundsHit(vec3 origin, vec3 direction, RayNode node, float maximum) {
    float nearDistance = 0.02;
    float farDistance = maximum;
    for (int axis = 0; axis < 3; ++axis) {
        if (abs(direction[axis]) < 0.0000001) {
            if (origin[axis] < node.minimum[axis] ||
                origin[axis] > node.maximum[axis]) return false;
        } else {
            float first = (node.minimum[axis] - origin[axis]) / direction[axis];
            float second = (node.maximum[axis] - origin[axis]) / direction[axis];
            if (first > second) {
                float temporary = first;
                first = second;
                second = temporary;
            }
            nearDistance = max(nearDistance, first);
            farDistance = min(farDistance, second);
            if (farDistance < nearDistance) return false;
        }
    }
    return true;
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

bool rayMeshClosest(vec3 origin, vec3 direction, uint root, uint nodeCount,
                    float maximum, out float distance, out vec3 normal) {
    uint stack[64];
    uint stackCount = 1;
    bool found = false;
    distance = maximum;
    stack[0] = root;
    while (stackCount != 0) {
        uint nodeIndex = stack[--stackCount];
        if (nodeIndex >= nodeCount) return found;
        RayNode node = rayNodes[nodeIndex];
        if (!rayBoundsHit(origin, direction, node, distance)) continue;
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
            if (stackCount + 2 > 64) return found;
            stack[stackCount++] = node.childAndRange.y;
            stack[stackCount++] = node.childAndRange.x;
        }
    }
    return found;
}

bool rayMeshHit(vec3 origin, vec3 direction, uint root, uint nodeCount) {
    uint stack[64];
    uint stackCount = 1;
    stack[0] = root;
    while (stackCount != 0) {
        uint nodeIndex = stack[--stackCount];
        if (nodeIndex >= nodeCount) return false;
        RayNode node = rayNodes[nodeIndex];
        if (!rayBoundsHit(origin, direction, node, 100000.0)) continue;
        if (node.childAndRange.w != 0) {
            uint end = node.childAndRange.z + node.childAndRange.w;
            for (uint offset = node.childAndRange.z; offset < end; ++offset) {
                uint triangleIndex = rayIndices[offset];
                if (rayTriangleHit(origin, direction,
                                   rayTriangles[triangleIndex], 100000.0))
                    return true;
            }
        } else {
            if (stackCount + 2 > 64) return false;
            stack[stackCount++] = node.childAndRange.y;
            stack[stackCount++] = node.childAndRange.x;
        }
    }
    return false;
}

float tracedVisibility(vec3 origin, vec3 direction, uint nodeCount,
                       uint instanceCount) {
    uint stack[64];
    uint stackCount = 1;
    stack[0] = 0;
    while (stackCount != 0) {
        uint nodeIndex = stack[--stackCount];
        if (nodeIndex >= nodeCount) return 1.0;
        RayNode node = rayNodes[nodeIndex];
        if (!rayBoundsHit(origin, direction, node, 100000.0)) continue;
        if (node.childAndRange.w != 0) {
            uint end = node.childAndRange.z + node.childAndRange.w;
            for (uint offset = node.childAndRange.z; offset < end; ++offset) {
                uint instanceIndex = rayIndices[offset];
                if (instanceIndex >= instanceCount) return 1.0;
                RayInstance instance = rayInstances[instanceIndex];
                vec4 point = vec4(origin, 1.0);
                vec4 vector = vec4(direction, 0.0);
                vec3 localOrigin = vec3(dot(instance.worldToLocal0, point),
                                        dot(instance.worldToLocal1, point),
                                        dot(instance.worldToLocal2, point));
                vec3 localDirection = vec3(dot(instance.worldToLocal0, vector),
                                           dot(instance.worldToLocal1, vector),
                                           dot(instance.worldToLocal2, vector));
                if (rayMeshHit(localOrigin, localDirection,
                               instance.meshAndFlags.x, nodeCount)) return 0.0;
            }
        } else {
            if (stackCount + 2 > 64) return 1.0;
            stack[stackCount++] = node.childAndRange.y;
            stack[stackCount++] = node.childAndRange.x;
        }
    }
    return 1.0;
}

bool tracedClosest(vec3 origin, vec3 direction, uint nodeCount,
                   uint instanceCount, out float distance, out vec3 normal) {
    uint stack[64];
    uint stackCount = 1;
    bool found = false;
    distance = 100000.0;
    stack[0] = 0;
    while (stackCount != 0) {
        uint nodeIndex = stack[--stackCount];
        if (nodeIndex >= nodeCount) return found;
        RayNode node = rayNodes[nodeIndex];
        if (!rayBoundsHit(origin, direction, node, distance)) continue;
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
            if (stackCount + 2 > 64) return found;
            stack[stackCount++] = node.childAndRange.y;
            stack[stackCount++] = node.childAndRange.x;
        }
    }
    return found;
}
