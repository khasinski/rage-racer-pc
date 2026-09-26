#ifndef PORT_NATIVE_STREAM_H
#define PORT_NATIVE_STREAM_H
#include "native_mesh_writer.h"
enum { RAGE_IMPORT_BATCH_GUARD = 65536 };

static uint16_t ImportRead16(const void *pointer) {
    const uint8_t *p = pointer;
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t ImportRead32(const void *pointer) {
    const uint8_t *p = pointer;
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}


static void ImportTextureWindow(uint32_t word,
                                    RageImportedTextureKey *texture) {
    uint32_t value, maskU, maskV, offsetU, offsetV;
    if ((word >> 24) != 0xE2) return;
    value = word & 0xFFFFFu;
    maskU = value & 0x1Fu;
    maskV = (value >> 5) & 0x1Fu;
    if (maskU == 0 && maskV == 0) return;
    offsetU = (value >> 10) & 0x1Fu;
    offsetV = (value >> 15) & 0x1Fu;
    texture->hasWindow = 1;
    texture->windowWidthU = (uint16_t)(256u - maskU * 8u);
    texture->windowWidthV = (uint16_t)(256u - maskV * 8u);
    texture->windowOffsetU = (uint16_t)((offsetU & maskU) * 8u);
    texture->windowOffsetV = (uint16_t)((offsetV & maskV) * 8u);
}

#endif
