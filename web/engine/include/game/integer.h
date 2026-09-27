#ifndef GAME_INTEGER_H
#define GAME_INTEGER_H

#include <stdint.h>

/* Convert the low bits of a wide calculation to a signed machine word
 * without relying on signed overflow or implementation-defined narrowing. */
static inline int32_t WrapSigned32(int64_t value) {
    uint32_t bits = (uint32_t)value;

    if (bits <= INT32_MAX) return (int32_t)bits;
    return (int32_t)((int64_t)bits - INT64_C(0x100000000));
}

static inline int16_t WrapSigned16(int64_t value) {
    uint16_t bits = (uint16_t)value;

    if (bits <= INT16_MAX) return (int16_t)bits;
    return (int16_t)((int32_t)bits - 0x10000);
}

#endif
