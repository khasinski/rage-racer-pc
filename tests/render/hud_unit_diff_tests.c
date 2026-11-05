#include "render/hud_unit_diff.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)
static uint8_t vram[1024 * 512 * 2];
int main(void) {
    uint8_t metric[RAGE_HUD_UNIT_BYTES], imperial[RAGE_HUD_UNIT_BYTES], state[RAGE_HUD_UNIT_BYTES];
    uint8_t encoded[RAGE_HUD_DIFF_MAX_BYTES], bad[RAGE_HUD_DIFF_MAX_BYTES];
    RageHudUnitDiff diff, decoded, sentinel;
    for (unsigned i = 0; i < sizeof(metric); ++i) metric[i] = (uint8_t)(i * 37u);
    memcpy(imperial, metric, sizeof(metric));
    imperial[17] ^= 0x10; /* One nibble, adjacent pixel unchanged. */
    imperial[RAGE_HUD_FACE_BYTES + 1] ^= 0x22; /* Day CLUT. */
    imperial[RAGE_HUD_UNIT_BYTES - 1] ^= 0x80; /* Night CLUT, STP included. */
    CHECK(RageHudUnitDiffBuild(&diff, 3, metric, imperial));
    size_t size = RageHudUnitDiffEncode(&diff, encoded, sizeof(encoded));
    CHECK(size == 139 + 3 * 3);
    CHECK(!RageHudUnitDiffEncode(&diff, bad, size - 1));
    CHECK(RageHudUnitDiffDecode(&decoded, encoded, size));
    CHECK(decoded.car == 3 && !memcmp(decoded.delta, diff.delta, sizeof(diff.delta)));
    memcpy(state, metric, sizeof(state));
    CHECK(RageHudUnitDiffApply(&decoded, 3, 1, state) == 2);
    CHECK(!memcmp(state, imperial, sizeof(state)));
    CHECK(RageHudUnitDiffApply(&decoded, 3, 1, state) == 1);
    CHECK(RageHudUnitDiffApply(&decoded, 3, 0, state) == 2);
    CHECK(!memcmp(state, metric, sizeof(state)));
    CHECK(RageHudUnitDiffApply(&decoded, 3, 0, state) == 1);
    CHECK(!RageHudUnitDiffApply(&decoded, 4, 1, state));
    CHECK(!memcmp(state, metric, sizeof(state)));
    state[100] ^= 1; /* Another layout/texture pack must be rejected atomically. */
    CHECK(!RageHudUnitDiffApply(&decoded, 3, 1, state));
    CHECK(state[100] == (uint8_t)(metric[100] ^ 1));
    state[100] ^= 1;
    decoded.delta[17] ^= 1; /* Correct source, corrupt patch: no partial writes. */
    CHECK(!RageHudUnitDiffApply(&decoded, 3, 1, state));
    CHECK(!memcmp(state, metric, sizeof(state)));
    memset(&sentinel, 0xa5, sizeof(sentinel));
    for (size_t truncated = 0; truncated < size; ++truncated) {
        decoded = sentinel;
        CHECK(!RageHudUnitDiffDecode(&decoded, encoded, truncated));
        CHECK(!memcmp(&decoded, &sentinel, sizeof(decoded)));
    }
    for (int kind = 0; kind < 7; ++kind) {
        memcpy(bad, encoded, size);
        if (kind == 0) bad[0] ^= 1;
        if (kind == 1) bad[8] = 13;
        if (kind == 2) bad[9] = 'z';
        if (kind == 3) bad[141] = 0; /* Zero delta record. */
        if (kind == 4) { bad[142] = bad[139]; bad[143] = bad[140]; } /* Duplicate. */
        if (kind == 5) { bad[139] = 0xff; bad[140] = 0xff; }
        if (kind == 6) bad[138] = 0xff;
        CHECK(!RageHudUnitDiffDecode(&decoded, bad, size));
    }
    /* Worst-case serialized diff; byte order and maximum capacity. */
    for (unsigned i = 0; i < sizeof(metric); ++i) imperial[i] = metric[i] ^ 0xff;
    CHECK(RageHudUnitDiffBuild(&diff, 12, metric, imperial));
    size = RageHudUnitDiffEncode(&diff, encoded, sizeof(encoded));
    CHECK(size == sizeof(encoded));
    CHECK(RageHudUnitDiffDecode(&decoded, encoded, size));
    memcpy(state, metric, sizeof(state));
    CHECK(RageHudUnitDiffApply(&decoded, 12, 1, state) == 2);
    CHECK(!memcmp(state, imperial, sizeof(state)));
    /* Empty diff is valid for dials whose artwork is identical. */
    CHECK(RageHudUnitDiffBuild(&diff, 9, metric, metric));
    CHECK(RageHudUnitDiffEncode(&diff, encoded, sizeof(encoded)) == 139);
    CHECK(RageHudUnitDiffApply(&diff, 9, 1, metric) == 1);
    CHECK(!RageHudUnitDiffBuild(&diff, 13, metric, metric));
    /* Extraction includes both CLUTs but excludes neighboring VRAM. */
    memset(vram, 0x44, sizeof(vram));
    vram[(112 * 1024 + 640) * 2] = 0x12;
    vram[(206 * 1024 + 640) * 2] = 0x34;
    vram[(207 * 1024 + 655) * 2 + 1] = 0x56;
    CHECK(RageHudUnitSnapshot(vram, sizeof(vram), state));
    CHECK(state[0] == 0x12 && state[1] == 0x44);
    CHECK(state[RAGE_HUD_FACE_BYTES] == 0x34);
    CHECK(state[RAGE_HUD_UNIT_BYTES - 1] == 0x56);
    CHECK(!RageHudUnitSnapshot(vram, sizeof(vram) - 1, state));
    puts("HUD diff: reversible byte-exact patch, source/target hashes, atomic rejection, strict format");
    return 0;
}
