#include "texture_upscale.h"

#include <stdlib.h>
#include <string.h>

size_t TextureUpscaleSize(uint32_t width, uint32_t height, uint32_t factor) {
    uint64_t outW, outH, pixels;
    if (width == 0 || height == 0 ||
        (factor != 1u && factor != 2u && factor != 4u))
        return 0;
    outW = (uint64_t)width * (uint64_t)factor;
    outH = (uint64_t)height * (uint64_t)factor;
    if (outW > 16384u || outH > 16384u) return 0;
    pixels = outW * outH;
    if (pixels > (uint64_t)(SIZE_MAX / 4u)) return 0;
    return (size_t)(pixels * 4u);
}

int TextureUpscaleDilateAlphaRGB(uint8_t *rgba, uint32_t width, uint32_t height,
                                 uint32_t passes) {
    static const int kDx[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    static const int kDy[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    size_t pixelCount;
    uint8_t *populated;
    uint8_t *nextPopulated;
    uint32_t pass, y, x;
    if (rgba == NULL || width == 0 || height == 0 ||
        (uint64_t)width * (uint64_t)height > (uint64_t)(SIZE_MAX / 4u))
        return 0;
    if (passes == 0) return 1;
    pixelCount = (size_t)width * (size_t)height;
    populated = malloc(pixelCount * 2u);
    if (populated == NULL) return 0;
    nextPopulated = populated + pixelCount;
    for (size_t i = 0; i < pixelCount; i++)
        populated[i] = rgba[i * 4u + 3u] > 0 ? 1u : 0u;
    for (pass = 0; pass < passes; pass++) {
        int changed = 0;
        memcpy(nextPopulated, populated, pixelCount);
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                size_t idx = (size_t)y * (size_t)width + (size_t)x;
                uint32_t sumR = 0, sumG = 0, sumB = 0, count = 0;
                int dir;
                if (populated[idx]) continue;
                for (dir = 0; dir < 8; dir++) {
                    int nx = (int)x + kDx[dir];
                    int ny = (int)y + kDy[dir];
                    size_t nIdx;
                    if (nx < 0 || ny < 0 || nx >= (int)width ||
                        ny >= (int)height)
                        continue;
                    nIdx = (size_t)ny * (size_t)width + (size_t)nx;
                    if (!populated[nIdx]) continue;
                    sumR += rgba[nIdx * 4u + 0u];
                    sumG += rgba[nIdx * 4u + 1u];
                    sumB += rgba[nIdx * 4u + 2u];
                    count++;
                }
                if (count > 0) {
                    rgba[idx * 4u + 0u] =
                        (uint8_t)((sumR + count / 2u) / count);
                    rgba[idx * 4u + 1u] =
                        (uint8_t)((sumG + count / 2u) / count);
                    rgba[idx * 4u + 2u] =
                        (uint8_t)((sumB + count / 2u) / count);
                    nextPopulated[idx] = 1u;
                    changed = 1;
                }
            }
        }
        memcpy(populated, nextPopulated, pixelCount);
        if (!changed) break;
    }
    free(populated);
    return 1;
}

static uint32_t WrapCoord(int coord, uint32_t base, uint32_t span,
                          uint32_t limit, int periodic) {
    if (span > 0 && span < limit && (uint32_t)base < limit) {
        int rel = coord - (int)base;
        int m = rel % (int)span;
        if (m < 0) m += (int)span;
        if ((uint32_t)base + (uint32_t)m < limit)
            return base + (uint32_t)m;
    }
    if (periodic && limit > 0) {
        int m = coord % (int)limit;
        if (m < 0) m += (int)limit;
        return (uint32_t)m;
    }
    if (coord < 0) return 0;
    if ((uint32_t)coord >= limit) return limit - 1u;
    return (uint32_t)coord;
}

static uint32_t ColorDist(const uint8_t *a, const uint8_t *b) {
    int dr = (int)a[0] - (int)b[0];
    int dg = (int)a[1] - (int)b[1];
    int db = (int)a[2] - (int)b[2];
    if (dr < 0) dr = -dr;
    if (dg < 0) dg = -dg;
    if (db < 0) db = -db;
    return (uint32_t)(dr * 3 + dg * 4 + db * 2);
}

/* Reconstructs one sub-pixel corner of center texel P facing orthogonal
 * neighbors H (horizontal), V (vertical) and diagonal neighbor D.
 * - Preserves crisp horizontal/vertical contrast edges.
 * - Reconstructs 45-degree diagonal edges when H and V match across D-P.
 * - Smoothly interpolates gentle shade ramps (car paint, asphalt, sky). */
static void ReconstructCorner(const uint8_t *P, const uint8_t *H,
                              const uint8_t *V, const uint8_t *D,
                              uint8_t *out) {
    uint32_t dHV, dDP, dPH, dPV, maxStep;
    int c;
    if (P[3] == 0) {
        out[0] = 0;
        out[1] = 0;
        out[2] = 0;
        out[3] = 0;
        return;
    }
    out[3] = P[3];
    /* Never blend with neighbors that had no valid RGB even after dilation. */
    if (H[3] == 0 && H[0] == 0 && H[1] == 0 && H[2] == 0) H = P;
    if (V[3] == 0 && V[0] == 0 && V[1] == 0 && V[2] == 0) V = P;
    if (D[3] == 0 && D[0] == 0 && D[1] == 0 && D[2] == 0) D = P;

    dHV = ColorDist(H, V);
    dDP = ColorDist(D, P);
    dPH = ColorDist(P, H);
    dPV = ColorDist(P, V);

    /* 1. Strong 45-degree diagonal edge: H and V are much closer to each other
     * than the D-P diagonal and P differs noticeably from both H and V. */
    if (dHV * 3u < dDP * 2u && dHV * 2u < dPH + dPV && dPH > 24u && dPV > 24u) {
        /* Weight 3/8 H + 3/8 V + 2/8 P for clean anti-aliased diagonal. */
        for (c = 0; c < 3; c++) {
            uint32_t val =
                (uint32_t)H[c] * 3u + (uint32_t)V[c] * 3u + (uint32_t)P[c] * 2u;
            out[c] = (uint8_t)((val + 4u) >> 3);
        }
        return;
    }

    /* 2. Continuous shade ramp / gradient: when neighbors vary gently without
     * a hard step edge, use bilinear sub-texel weights (9:3:3:1) to eliminate
     * 5-bit PS1 color banding on car bodies, roads and sky clouds. */
    maxStep = dPH > dPV ? dPH : dPV;
    if (maxStep > 0 && maxStep <= 144u) {
        const uint8_t *useD = ColorDist(P, D) <= 144u ? D : P;
        for (c = 0; c < 3; c++) {
            uint32_t val = (uint32_t)P[c] * 9u + (uint32_t)H[c] * 3u +
                           (uint32_t)V[c] * 3u + (uint32_t)useD[c];
            out[c] = (uint8_t)((val + 8u) >> 4);
        }
        return;
    }

    /* 3. Directional 1D ramp: smooth along H or V alone if one axis is a gentle
     * ramp while the other axis is a sharp boundary. */
    if (dPH > 0 && dPH <= 108u && dPV > 144u) {
        for (c = 0; c < 3; c++)
            out[c] = (uint8_t)(((uint32_t)P[c] * 3u + (uint32_t)H[c] + 2u) >> 2);
        return;
    }
    if (dPV > 0 && dPV <= 108u && dPH > 144u) {
        for (c = 0; c < 3; c++)
            out[c] = (uint8_t)(((uint32_t)P[c] * 3u + (uint32_t)V[c] + 2u) >> 2);
        return;
    }

    /* 4. Sharp horizontal/vertical edge or uniform texel: keep crisp. */
    out[0] = P[0];
    out[1] = P[1];
    out[2] = P[2];
}

static int UpscalePass2x(const uint8_t *src, uint32_t width, uint32_t height,
                         const RageTextureUpscaleWindow *window, uint8_t *dst) {
    size_t srcBytes = (size_t)width * (size_t)height * 4u;
    uint32_t outStride = width * 2u * 4u;
    uint8_t *working = malloc(srcBytes);
    uint32_t y, x;
    uint32_t winOffU = 0, winOffV = 0, winW = 0, winH = 0;
    int wrapH = 0;
    if (working == NULL) return 0;
    memcpy(working, src, srcBytes);
    (void)TextureUpscaleDilateAlphaRGB(working, width, height, 2u);

    if (window != NULL) {
        winOffU = window->offsetU;
        winOffV = window->offsetV;
        winW = window->widthU;
        winH = window->heightV;
        wrapH = window->wrapHorizontal;
    }

    for (y = 0; y < height; y++) {
        uint32_t tileBaseV = winOffV;
        uint32_t tileSpanV = 0;
        if (winH > 0 && winH < height && y >= winOffV) {
            tileBaseV = winOffV + ((y - winOffV) / winH) * winH;
            if (tileBaseV + winH <= height) tileSpanV = winH;
        }
        uint32_t yN = WrapCoord((int)y - 1, tileBaseV, tileSpanV, height, 0);
        uint32_t yS = WrapCoord((int)y + 1, tileBaseV, tileSpanV, height, 0);
        uint8_t *dstRow0 = dst + (size_t)(y * 2u) * (size_t)outStride;
        uint8_t *dstRow1 = dst + (size_t)(y * 2u + 1u) * (size_t)outStride;

        for (x = 0; x < width; x++) {
            uint32_t tileBaseU = winOffU;
            uint32_t tileSpanU = 0;
            if (winW > 0 && winW < width && x >= winOffU) {
                tileBaseU = winOffU + ((x - winOffU) / winW) * winW;
                if (tileBaseU + winW <= width) tileSpanU = winW;
            }
            uint32_t xW =
                WrapCoord((int)x - 1, tileBaseU, tileSpanU, width, wrapH);
            uint32_t xE =
                WrapCoord((int)x + 1, tileBaseU, tileSpanU, width, wrapH);

            const uint8_t *NW =
                working + ((size_t)yN * (size_t)width + (size_t)xW) * 4u;
            const uint8_t *N =
                working + ((size_t)yN * (size_t)width + (size_t)x) * 4u;
            const uint8_t *NE =
                working + ((size_t)yN * (size_t)width + (size_t)xE) * 4u;
            const uint8_t *W =
                working + ((size_t)y * (size_t)width + (size_t)xW) * 4u;
            const uint8_t *P =
                working + ((size_t)y * (size_t)width + (size_t)x) * 4u;
            const uint8_t *E =
                working + ((size_t)y * (size_t)width + (size_t)xE) * 4u;
            const uint8_t *SW =
                working + ((size_t)yS * (size_t)width + (size_t)xW) * 4u;
            const uint8_t *S =
                working + ((size_t)yS * (size_t)width + (size_t)x) * 4u;
            const uint8_t *SE =
                working + ((size_t)yS * (size_t)width + (size_t)xE) * 4u;

            ReconstructCorner(P, W, N, NW, dstRow0 + (size_t)(x * 2u) * 4u);
            ReconstructCorner(P, E, N, NE, dstRow0 + (size_t)(x * 2u + 1u) * 4u);
            ReconstructCorner(P, W, S, SW, dstRow1 + (size_t)(x * 2u) * 4u);
            ReconstructCorner(P, E, S, SE, dstRow1 + (size_t)(x * 2u + 1u) * 4u);
        }
    }
    free(working);
    return 1;
}

int TextureUpscaleRGBA(const uint8_t *src, uint32_t width, uint32_t height,
                       uint32_t factor, const RageTextureUpscaleWindow *window,
                       uint8_t *dst, size_t dstCapacity) {
    size_t needed = TextureUpscaleSize(width, height, factor);
    if (src == NULL || dst == NULL || needed == 0 || dstCapacity < needed)
        return 0;
    if (factor == 1u) {
        if (src != dst) memcpy(dst, src, needed);
        return 1;
    }
    if (factor == 2u) {
        return UpscalePass2x(src, width, height, window, dst);
    }
    if (factor == 4u) {
        size_t midBytes = TextureUpscaleSize(width, height, 2u);
        uint8_t *mid = malloc(midBytes);
        RageTextureUpscaleWindow midWindow = {0};
        const RageTextureUpscaleWindow *midWindowPtr = NULL;
        int ok;
        if (mid == NULL) return 0;
        if (!UpscalePass2x(src, width, height, window, mid)) {
            free(mid);
            return 0;
        }
        if (window != NULL) {
            midWindow.offsetU = (uint16_t)(window->offsetU * 2u);
            midWindow.offsetV = (uint16_t)(window->offsetV * 2u);
            midWindow.widthU = (uint16_t)(window->widthU * 2u);
            midWindow.heightV = (uint16_t)(window->heightV * 2u);
            midWindow.wrapHorizontal = window->wrapHorizontal;
            midWindowPtr = &midWindow;
        }
        ok = UpscalePass2x(mid, width * 2u, height * 2u, midWindowPtr, dst);
        free(mid);
        return ok;
    }
    return 0;
}
