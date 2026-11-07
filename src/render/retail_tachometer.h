#ifndef RAGE_RETAIL_TACHOMETER_H
#define RAGE_RETAIL_TACHOMETER_H
#include <stdint.h>
/* DrawPlayerTachometer's evaluated packet fields, in 320x240 coordinates. */
enum {
    RAGE_TACHO_VISIBLE=0, RAGE_TACHO_NEEDLE=1, RAGE_TACHO_NEEDLE_COLOR=9,
    RAGE_TACHO_FACE=12, RAGE_TACHO_GEAR=16, RAGE_TACHO_SPEED=19,
    RAGE_TACHO_DIGIT_CLUT=22, RAGE_TACHO_SHIFT_LIGHT=23,
    RAGE_TACHO_FACE_SIZE=26, RAGE_TACHO_WORDS=32,
    RAGE_TACHO_ATLAS_WIDTH=256, RAGE_TACHO_ATLAS_HEIGHT=112
};
typedef struct RageRetailTachometer {
    int32_t words[RAGE_TACHO_WORDS];
    const uint8_t *atlas;
    uint64_t revision;
} RageRetailTachometer;
typedef struct RageTachometerQuad {
    float xy[4][2], color[4];
    int textured, u,v,w,h;
} RageTachometerQuad;
/* Seven quads, in retail ordering-table order: shift lamp, face, speed
 * digits, gear, needle. The caller applies its usual HUD edge anchor. */
int RenderRetailTachometerBuild(const RageRetailTachometer *,int edgeOffset,
                               RageTachometerQuad quads[7]);
#endif
