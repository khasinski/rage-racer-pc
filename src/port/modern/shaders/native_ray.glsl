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

#ifndef RAY_NODE_BINDING
#define RAY_NODE_BINDING 2
#define RAY_TRIANGLE_BINDING 3
#define RAY_INDEX_BINDING 4
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

float tracedVisibility(vec3 origin, vec3 direction, uint nodeCount) {
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
                uint triangleIndex = rayIndices[offset];
                if (rayTriangleHit(origin, direction,
                                   rayTriangles[triangleIndex], 100000.0))
                    return 0.0;
            }
        } else {
            if (stackCount + 2 > 64) return 1.0;
            stack[stackCount++] = node.childAndRange.y;
            stack[stackCount++] = node.childAndRange.x;
        }
    }
    return 1.0;
}
