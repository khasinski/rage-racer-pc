#ifndef RAGE_PORT_PSYQ_GPU_H
#define RAGE_PORT_PSYQ_GPU_H

#include <libgpu.h>

typedef RECT Rect;
typedef DRAWENV DrawEnv;
typedef DISPENV DispEnv;
typedef DR_MODE DrawPacket;

typedef struct GpuRectPacked {
    unsigned int xy;
    short w;
    short h;
} GpuRectPacked;

#define CLUT_STP_BIT 0x8000
#define POLY_FT4_CODE 0x2C
#define POLY_GT4_CODE 0x3C

#endif
