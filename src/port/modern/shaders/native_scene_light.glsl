struct SpotLight {
    vec4 positionRange;
    vec4 directionOuter;
    vec4 colorInner;
};
layout(set = 3, binding = 0, std140) uniform NativeSceneLight {
    vec4 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 skyTop;
    vec4 skyHorizon;
    vec4 skyBottom;
    vec4 shadowParams; // cascade count, 1 / atlas size, reserved, reserved
    vec4 shadowTexel;  // world units per texel of each cascade
    vec4 shadowRows[12]; // per cascade: atlas u, atlas v, depth as affine rows
    vec4 spotCount;
    SpotLight spots[72];
} sceneLight;
