layout(set = 1, binding = LOCAL_BINDING, std140) uniform NativeLocal {
    vec4 positionMode;
    vec4 scaleFog;
    vec4 rotation0;
    vec4 rotation1;
    vec4 rotation2;
} localGeometry;

vec3 rotateLocal(vec3 v) {
    if (localGeometry.positionMode.w > 1.5) {
        return vec3(dot(localGeometry.rotation0.xyz, v),
                    dot(localGeometry.rotation1.xyz, v),
                    dot(localGeometry.rotation2.xyz, v));
    }
    vec3 c = localGeometry.rotation0.xyz;
    vec3 s = localGeometry.rotation1.xyz;
    v.yz = vec2(v.y * c.x - v.z * s.x, v.y * s.x + v.z * c.x);
    v.xz = vec2(v.x * c.y + v.z * s.y, -v.x * s.y + v.z * c.y);
    v.xy = vec2(v.x * c.z - v.y * s.z, v.x * s.z + v.y * c.z);
    return v;
}

void transformLocal(inout vec3 position, inout vec3 normal, inout vec4 fog) {
    if (localGeometry.positionMode.w < 0.5) return;
    position = rotateLocal(position * localGeometry.scaleFog.xyz) + localGeometry.positionMode.xyz;
    normal = rotateLocal(normal);
    fog = vec4(position, localGeometry.scaleFog.w);
    float magnitude = length(normal);
    if (magnitude > 0.0) position += normal * (localGeometry.rotation0.w / magnitude);
}
