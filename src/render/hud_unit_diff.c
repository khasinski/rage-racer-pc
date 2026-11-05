#include "hud_unit_diff.h"
#include "content/sha256.h"
#include <string.h>

static void Hash(const uint8_t *bytes, char out[65]) {
    Sha256 hash;
    Sha256Init(&hash);
    Sha256Update(&hash, bytes, RAGE_HUD_UNIT_BYTES);
    Sha256Finish(&hash, out);
}
static int ValidHash(const char hash[65]) {
    for (int i = 0; i < 64; ++i)
        if (!hash[i] || !strchr("0123456789abcdef", hash[i])) return 0;
    return hash[64] == '\0';
}
int RageHudUnitSnapshot(const uint8_t *vram, size_t size,
                        uint8_t out[RAGE_HUD_UNIT_BYTES]) {
    if (!vram || !out || size != 1024u * 512u * 2u) return 0;
    for (unsigned y = 0; y < 94; ++y)
        memcpy(out + y * 48u, vram + ((112u + y) * 1024u + 640u) * 2u, 48);
    for (unsigned night = 0; night < 2; ++night)
        memcpy(out + RAGE_HUD_FACE_BYTES + night * 32u,
               vram + ((206u + night) * 1024u + 640u) * 2u, 32);
    return 1;
}
int RageHudUnitDiffBuild(RageHudUnitDiff *out, unsigned car,
                        const uint8_t metric[RAGE_HUD_UNIT_BYTES],
                        const uint8_t imperial[RAGE_HUD_UNIT_BYTES]) {
    if (!out || !metric || !imperial || car >= 13) return 0;
    out->car = car;
    Hash(metric, out->metricHash);
    Hash(imperial, out->imperialHash);
    for (unsigned i = 0; i < RAGE_HUD_UNIT_BYTES; ++i) out->delta[i] = metric[i] ^ imperial[i];
    return 1;
}
size_t RageHudUnitDiffEncode(const RageHudUnitDiff *diff, uint8_t *out, size_t capacity) {
    if (!diff || diff->car >= 13 || !ValidHash(diff->metricHash) ||
        !ValidHash(diff->imperialHash)) return 0;
    unsigned count = 0;
    for (unsigned i = 0; i < RAGE_HUD_UNIT_BYTES; ++i) count += diff->delta[i] != 0;
    size_t size = 139u + (size_t)count * 3u;
    if (!out || capacity < size) return 0;
    memcpy(out, "RRHUDD01", 8);
    out[8] = (uint8_t)diff->car;
    memcpy(out + 9, diff->metricHash, 64);
    memcpy(out + 73, diff->imperialHash, 64);
    out[137] = (uint8_t)count;
    out[138] = (uint8_t)(count >> 8);
    size_t offset = 139;
    for (unsigned i = 0; i < RAGE_HUD_UNIT_BYTES; ++i) if (diff->delta[i]) {
        out[offset++] = (uint8_t)i;
        out[offset++] = (uint8_t)(i >> 8);
        out[offset++] = diff->delta[i];
    }
    return size;
}
int RageHudUnitDiffDecode(RageHudUnitDiff *out, const uint8_t *bytes, size_t size) {
    RageHudUnitDiff parsed = {0};
    if (!out || !bytes || size < 139 || memcmp(bytes, "RRHUDD01", 8) || bytes[8] >= 13) return 0;
    unsigned count = (unsigned)bytes[137] | (unsigned)bytes[138] << 8;
    if (count > RAGE_HUD_UNIT_BYTES || size != 139u + (size_t)count * 3u) return 0;
    parsed.car = bytes[8];
    memcpy(parsed.metricHash, bytes + 9, 64);
    memcpy(parsed.imperialHash, bytes + 73, 64);
    if (!ValidHash(parsed.metricHash) || !ValidHash(parsed.imperialHash)) return 0;
    unsigned previous = 0;
    for (unsigned i = 0; i < count; ++i) {
        size_t at = 139u + (size_t)i * 3u;
        unsigned index = (unsigned)bytes[at] | (unsigned)bytes[at+1] << 8;
        if (index >= RAGE_HUD_UNIT_BYTES || (i && index <= previous) || !bytes[at+2]) return 0;
        parsed.delta[index] = bytes[at+2];
        previous = index;
    }
    *out = parsed;
    return 1;
}
int RageHudUnitDiffApply(const RageHudUnitDiff *diff, unsigned car, int imperial,
                        uint8_t snapshot[RAGE_HUD_UNIT_BYTES]) {
    char hash[65];
    uint8_t target[RAGE_HUD_UNIT_BYTES];
    if (!diff || !snapshot || car != diff->car || car >= 13 ||
        !ValidHash(diff->metricHash) || !ValidHash(diff->imperialHash)) return 0;
    const char *wanted = imperial ? diff->imperialHash : diff->metricHash;
    const char *source = imperial ? diff->metricHash : diff->imperialHash;
    Hash(snapshot, hash);
    if (!strcmp(hash, wanted)) return 1;
    if (strcmp(hash, source)) return 0;
    for (unsigned i = 0; i < RAGE_HUD_UNIT_BYTES; ++i) target[i] = snapshot[i] ^ diff->delta[i];
    Hash(target, hash);
    if (strcmp(hash, wanted)) return 0;
    memcpy(snapshot, target, sizeof(target));
    return 2;
}
