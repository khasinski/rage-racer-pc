#include "disc_stage_validation.h"

#include <string.h>

static uint32_t RotateLeft(uint32_t value, unsigned count) {
    return (value << count) | (value >> (32u - count));
}

static uint32_t ReadBe32(const uint8_t *bytes) {
    return ((uint32_t)bytes[0] << 24) | ((uint32_t)bytes[1] << 16) |
           ((uint32_t)bytes[2] << 8) | bytes[3];
}

static void WriteBe32(uint8_t *bytes, uint32_t value) {
    bytes[0] = (uint8_t)(value >> 24);
    bytes[1] = (uint8_t)(value >> 16);
    bytes[2] = (uint8_t)(value >> 8);
    bytes[3] = (uint8_t)value;
}

static void Transform(uint32_t state[5], const uint8_t block[64]) {
    uint32_t words[80];
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3], e = state[4];
    unsigned index;
    for (index = 0; index < 16; ++index) words[index] = ReadBe32(block + index * 4);
    for (index = 16; index < 80; ++index)
        words[index] = RotateLeft(words[index - 3] ^ words[index - 8] ^
                                  words[index - 14] ^ words[index - 16], 1);
    for (index = 0; index < 80; ++index) {
        uint32_t f, k, next;
        if (index < 20) {
            f = (b & c) | ((~b) & d); k = 0x5A827999u;
        } else if (index < 40) {
            f = b ^ c ^ d; k = 0x6ED9EBA1u;
        } else if (index < 60) {
            f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDCu;
        } else {
            f = b ^ c ^ d; k = 0xCA62C1D6u;
        }
        next = RotateLeft(a, 5) + f + e + k + words[index];
        e = d; d = c; c = RotateLeft(b, 30); b = a; a = next;
    }
    state[0] += a; state[1] += b; state[2] += c; state[3] += d; state[4] += e;
}

void RageDiscStageSha1(const void *data, size_t size,
                       uint8_t digest[RAGE_DISC_STAGE_SHA1_BYTES]) {
    const uint8_t *bytes = data;
    uint32_t state[5] = {0x67452301u, 0xEFCDAB89u, 0x98BADCFEu,
                         0x10325476u, 0xC3D2E1F0u};
    uint8_t block[64] = {0};
    uint64_t bits = (uint64_t)size * 8u;
    size_t index = 0;
    while (size - index >= sizeof(block)) {
        Transform(state, bytes + index);
        index += sizeof(block);
    }
    if (size != index) memcpy(block, bytes + index, size - index);
    block[size - index] = 0x80;
    if (size - index >= 56) {
        Transform(state, block);
        memset(block, 0, sizeof(block));
    }
    block[56] = (uint8_t)(bits >> 56); block[57] = (uint8_t)(bits >> 48);
    block[58] = (uint8_t)(bits >> 40); block[59] = (uint8_t)(bits >> 32);
    block[60] = (uint8_t)(bits >> 24); block[61] = (uint8_t)(bits >> 16);
    block[62] = (uint8_t)(bits >> 8); block[63] = (uint8_t)bits;
    Transform(state, block);
    for (index = 0; index < 5; ++index) WriteBe32(digest + index * 4, state[index]);
}

void RageDiscStageSha1Hex(const uint8_t digest[RAGE_DISC_STAGE_SHA1_BYTES],
                          char hex[RAGE_DISC_STAGE_SHA1_BYTES * 2 + 1]) {
    static const char digits[] = "0123456789abcdef";
    size_t index;
    for (index = 0; index < RAGE_DISC_STAGE_SHA1_BYTES; ++index) {
        hex[index * 2] = digits[digest[index] >> 4];
        hex[index * 2 + 1] = digits[digest[index] & 15u];
    }
    hex[RAGE_DISC_STAGE_SHA1_BYTES * 2] = '\0';
}

int RageDiscStageValidatePsxExe(const void *data, size_t size,
                                const char *expectedSha1) {
    uint8_t digest[RAGE_DISC_STAGE_SHA1_BYTES];
    char actual[RAGE_DISC_STAGE_SHA1_BYTES * 2 + 1];
    if (!data || !expectedSha1 || size < 0x800 ||
        memcmp(data, "PS-X EXE", 8) != 0 || strlen(expectedSha1) != 40)
        return 0;
    RageDiscStageSha1(data, size, digest);
    RageDiscStageSha1Hex(digest, actual);
    return strcmp(actual, expectedSha1) == 0;
}
