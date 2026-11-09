/* Integrate extinction along the camera ray. The air term gives distant
 * scenery the horizon's colour; exponential height fog adds low-lying haze.
 * Both share one transmittance and are applied AFTER texture and lighting. */
vec3 applyAtmosphere(vec3 surface, vec3 position, vec3 environment,
                     float enabled) {
    if (sceneLight.atmosphereRange.w < 0.5 || enabled < 0.5) return surface;
    vec3 ray = position - sceneLight.atmosphereEye.xyz;
    float distance = length(ray);
    if (distance < 0.001) return surface;
    float nearDistance = sceneLight.atmosphereEye.w * 0.25;
    float farDistance = sceneLight.atmosphereRange.x;
    float airDepth = max(distance - nearDistance, 0.0) *
                     (0.85 / max(farDistance - nearDistance, 1.0));
    float heightScale = sceneLight.atmosphereRange.y;
    float ground = sceneLight.atmosphereRange.z;
    float h0 = clamp((sceneLight.atmosphereEye.y - ground) / heightScale, -2.0, 20.0);
    float h1 = clamp((position.y - ground) / heightScale, -2.0, 20.0);
    float difference = h1 - h0;
    /* Stable horizontal-ray limit, including the camera exactly at sea level. */
    float averageDensity = abs(difference) < 0.001
        ? exp(-0.5 * (h0 + h1))
        : (exp(-h0) - exp(-h1)) / difference;
    float heightDepth = max(distance - nearDistance * 0.5, 0.0) /
                        max(farDistance, 1.0) * 0.70 * averageDensity;
    /* Authored light zones keep close enclosed scenery from turning into a
     * bright outdoor fog bank. No geometry readback or extra depth pass. */
    float openness = clamp(max(environment.r, max(environment.g, environment.b)), 0.0, 1.0);
    heightDepth *= openness;
    airDepth *= mix(0.25, 1.0, openness);
    float opticalDepth = airDepth + heightDepth;
    if (opticalDepth < 0.00001) return surface;
    vec3 direction = ray / distance;
    vec3 airColor = mix(sceneLight.skyHorizon.rgb, sceneLight.skyTop.rgb,
                        smoothstep(0.0, 0.7, max(direction.y, 0.0)));
    vec3 scattering = (airColor * airDepth +
        sceneLight.atmosphereFogColor.rgb * heightDepth) / opticalDepth;
    return mix(surface, scattering, 1.0 - exp(-opticalDepth));
}
