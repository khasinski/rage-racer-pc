/* Sun visibility from the cascaded shadow atlas. The includer declares
 * sceneLight (native_scene_light.glsl) and a comparison sampler `shadowMap`.
 *
 * Cascades are nested around the player, finest first. A fragment uses the
 * finest cascade that contains it and fades into the next one near the tile
 * edge, so cascade changes never show as a seam. Receivers are moved along
 * their normal by a fraction of a texel, which removes self-shadowing acne
 * without a depth bias large enough to detach shadows from their casters. */

#define SHADOW_BLEND_WIDTH 0.08
#define SHADOW_DEPTH_BIAS 0.00012
#define SHADOW_NORMAL_OFFSET_TEXELS 1.5

/* Tent-weighted 3x3 grid of hardware-filtered comparisons: a smooth penumbra a
 * few texels wide that does not change from frame to frame. */
float shadowFiltered(vec3 coord) {
    float texel = sceneLight.shadowParams.y;
    float sum = 0.0;
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            float weight = (x == 0 ? 2.0 : 1.0) * (y == 0 ? 2.0 : 1.0);
            sum += weight * texture(shadowMap,
                vec3(coord.xy + vec2(float(x), float(y)) * texel, coord.z));
        }
    }
    return sum / 16.0;
}

/* Visibility within one cascade, or -1 when the fragment is outside it. The
 * distance to the tile edge (0 at the edge, 0.5 at the centre) is returned
 * through `edge` for blending. */
float shadowCascade(int cascade, vec3 position, vec3 normal,
                    float sine, out float edge) {
    vec3 offset = position + normal *
        (sceneLight.shadowTexel[cascade] * SHADOW_NORMAL_OFFSET_TEXELS * sine);
    vec4 u = sceneLight.shadowRows[cascade * 3];
    vec4 v = sceneLight.shadowRows[cascade * 3 + 1];
    vec4 d = sceneLight.shadowRows[cascade * 3 + 2];
    vec3 coord = vec3(dot(u.xyz, offset) + u.w, dot(v.xyz, offset) + v.w,
                      dot(d.xyz, offset) + d.w);
    vec2 tile = coord.xy * 2.0 - vec2(float(cascade & 1), float(cascade >> 1));
    vec2 inner = min(tile, 1.0 - tile);
    edge = min(inner.x, inner.y);
    if (edge <= 0.0 || coord.z <= 0.0 || coord.z >= 1.0) return -1.0;
    coord.z -= SHADOW_DEPTH_BIAS;
    return shadowFiltered(coord);
}

float sunVisibility(vec3 position, vec3 normal) {
    int count = int(sceneLight.shadowParams.x + 0.5);
    float sine = sqrt(max(1.0 - pow(dot(normal,
        normalize(sceneLight.direction.xyz)), 2.0), 0.0));
    for (int cascade = 0; cascade < count; ++cascade) {
        float edge;
        float visibility = shadowCascade(cascade, position, normal, sine, edge);
        if (visibility < 0.0) continue;
        float blend = 1.0 - smoothstep(0.0, SHADOW_BLEND_WIDTH, edge);
        if (blend > 0.0) {
            float next = 1.0;
            if (cascade + 1 < count) {
                float nextEdge;
                float candidate = shadowCascade(
                    cascade + 1, position, normal, sine, nextEdge);
                if (candidate >= 0.0) next = candidate;
            }
            visibility = mix(visibility, next, blend);
        }
        return visibility;
    }
    return 1.0;
}
