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

static inline int RageLoadImage(void *rect, void *pixels) {
    return LoadImage((RECT *)rect, (u_long *)pixels);
}
static inline int RageStoreImage(void *rect, void *pixels) {
    return StoreImage((RECT *)rect, (u_long *)pixels);
}
static inline int RageMoveImage(void *rect, int x, int y) {
    return MoveImage((RECT *)rect, x, y);
}
static inline void RageSetDrawArea(void *packet, void *rect) {
    SetDrawArea((DR_AREA *)packet, (RECT *)rect);
}
static inline void RageSetDrawMode(
    void *packet, int dfe, int dtd, int tpage, void *window) {
    SetDrawMode((DR_MODE *)packet, dfe, dtd, tpage, (RECT *)window);
}
static inline void RageSetSprt(void *packet) { SetSprt((SPRT *)packet); }
static inline void RageSetSprt8(void *packet) { SetSprt8((SPRT_8 *)packet); }
static inline void RageSetPolyFT4(void *packet) { SetPolyFT4((POLY_FT4 *)packet); }
static inline void RageSetPolyG4(void *packet) { SetPolyG4((POLY_G4 *)packet); }

#define LoadImage RageLoadImage
#define StoreImage RageStoreImage
#define MoveImage RageMoveImage
#define SetDrawArea RageSetDrawArea
#define SetDrawMode RageSetDrawMode
#define SetSprt RageSetSprt
#define SetSprt8 RageSetSprt8
#define SetPolyFT4 RageSetPolyFT4
#define SetPolyG4 RageSetPolyG4

#endif
