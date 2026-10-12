#include "car_bank.h"

#include <stdint.h>
#include <string.h>

static uint32_t Read32(const unsigned char *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static int Geometry(const unsigned char *p, size_t size, int rival,
                    size_t *offset, size_t *length) {
    size_t header = rival ? 44u : 40u;
    if (!p || size < header) return 0;
    *offset = Read32(p + (rival ? 12 : 32));
    if (rival) {
        size_t end = Read32(p + 16);
        if (end < *offset) return 0;
        *length = end - *offset;
    } else {
        *length = Read32(p + 24);
    }
    return *offset >= header && *offset <= size && *length >= 12 &&
           *length <= size - *offset;
}

int CarBankGeometryEqual(const void *left, size_t leftSize,
                         const void *right, size_t rightSize, int rival) {
    size_t a, b, an, bn;
    return Geometry(left, leftSize, rival, &a, &an) &&
           Geometry(right, rightSize, rival, &b, &bn) && an == bn &&
           !memcmp((const unsigned char *)left + a,
                   (const unsigned char *)right + b, an);
}
