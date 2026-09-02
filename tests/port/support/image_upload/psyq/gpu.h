#ifndef TEST_IMAGE_UPLOAD_PSYQ_GPU_H
#define TEST_IMAGE_UPLOAD_PSYQ_GPU_H

#include "common.h"

typedef struct Rect {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
} Rect;

typedef Rect RECT;
typedef unsigned long u_long;

typedef struct DrawPacket DrawPacket;
typedef struct SPRT SPRT;
typedef struct SPRT_8 SPRT_8;
typedef struct TILE TILE;
typedef struct LINE_F2 LINE_F2;
typedef struct LINE_F3 LINE_F3;
typedef struct LINE_G2 LINE_G2;
typedef struct POLY_F3 POLY_F3;
typedef struct POLY_F4 POLY_F4;
typedef struct POLY_FT4 POLY_FT4;
typedef struct POLY_G4 POLY_G4;

typedef struct GpuRectPacked {
    u32 xy;
    s16 w;
    s16 h;
} GpuRectPacked;

#define CLUT_STP_BIT 0x8000

void LoadImage(Rect *rect, void *data);
void StoreImage(Rect *rect, void *data);
long MoveImage(GpuRectPacked *rect, unsigned long x, unsigned long y);
void DrawSync(long mode);

#endif
