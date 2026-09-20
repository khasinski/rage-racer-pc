/* Local lights share linear scene space with the sun. Range and cone tests
 * reject fragments before normalization; no extra BVH traversal is needed. */
vec3 spotLighting(vec3 position, vec3 normal) {
    vec3 result = vec3(0.0);
    int count = clamp(int(sceneLight.spotCount.x), 0, 72);
    for (int i = 0; i < count; ++i) {
        SpotLight lamp = sceneLight.spots[i];
        vec3 offset = position - lamp.positionRange.xyz;
        float distanceSquared = dot(offset, offset);
        float rangeSquared = lamp.positionRange.w * lamp.positionRange.w;
        if (distanceSquared >= rangeSquared || distanceSquared < 0.000001)
            continue;
        vec3 direction = offset * inversesqrt(distanceSquared);
        float cone = dot(direction, lamp.directionOuter.xyz);
        if (cone <= lamp.directionOuter.w) continue;
        float edge = smoothstep(lamp.directionOuter.w, lamp.colorInner.w, cone);
        float falloff = 1.0 - distanceSquared / rangeSquared;
        float facing = max(dot(normal, -direction), 0.0);
        result += lamp.colorInner.rgb * edge * falloff * falloff * facing;
    }
    return result;
}
