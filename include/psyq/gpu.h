#ifndef RAGE_PC_PSYQ_GPU_H
#define RAGE_PC_PSYQ_GPU_H

#include <sys/types.h>

#include "common.h"
#include "psyq/gpu_types.h"

typedef struct DispEnv {
    Rect disp;
    Rect screen;
    u_char isinter;
    u_char isrgb24;
    u_char pad0;
    u_char pad1;
} DispEnv;

typedef struct DrawEnvPacket {
    u_long tag;
    u_long code[15];
} DrawEnvPacket;

typedef struct DrawEnv {
    Rect clip;
    short ofs[2];
    Rect tw;
    u_short tpage;
    u_char dtd;
    u_char dfe;
    u_char isbg;
    u_char r0;
    u_char g0;
    u_char b0;
    DrawEnvPacket packet;
} DrawEnv;

typedef char DrawEnvSizeCheck[sizeof(DrawEnv) == 0x5C ? 1 : -1];
typedef char DispEnvSizeCheck[sizeof(DispEnv) == 0x14 ? 1 : -1];

s32 SetGraphDebug(u8 level);
DrawEnv *PutDrawEnv(DrawEnv *env);
DispEnv *PutDispEnv(DispEnv *env);
void DrawOTagEnv(void *ot, DrawEnv *env);
void DrawOTag(void *ot);
u_char *MemCopy(u_char *dst, u_char *src, long count);

/*
 * The libgpu primitive packets the game builds, in PSY-Q layout: a 4-byte
 * ordering-table tag that AddPrim links, then the packed command word (rgb of
 * vertex 0 + the primitive code the Set* helpers stamp in).
 */
typedef struct P_TAG {
    /* PSY-Z keeps full native links so ordering tables work on 64-bit hosts. */
    u_long addr;
    u_long len;
    u_char r0;
    u_char g0;
    u_char b0;
    u_char code;
} P_TAG;

/* Flat triangle, 0x14 bytes. Built by SetPolyF3. */
typedef struct POLY_F3 {
    P_TAG t;
    short x0;
    short y0;
    short x1;
    short y1;
    short x2;
    short y2;
} POLY_F3;

/* Flat quad, 0x18 bytes. Built by SetPolyF4. */
typedef struct POLY_F4 {
    P_TAG t;
    short x0;
    short y0;
    short x1;
    short y1;
    short x2;
    short y2;
    short x3;
    short y3;
} POLY_F4;

typedef struct POLY_G4 {
    P_TAG t;
    short x0;
    short y0;
    u_char r1;
    u_char g1;
    u_char b1;
    u_char pad0;
    short x1;
    short y1;
    u_char r2;
    u_char g2;
    u_char b2;
    u_char pad1;
    short x2;
    short y2;
    u_char r3;
    u_char g3;
    u_char b3;
    u_char pad2;
    short x3;
    short y3;
} POLY_G4;

/* Textured quad, 0x28 bytes. Built by SetPolyFT4. */
typedef struct POLY_FT4 {
    P_TAG t;
    short x0;
    short y0;
    u_char u0;
    u_char v0;
    u_short clut;
    short x1;
    short y1;
    u_char u1;
    u_char v1;
    u_short tpage;
    short x2;
    short y2;
    u_char u2;
    u_char v2;
    u_short pad1E;
    short x3;
    short y3;
    u_char u3;
    u_char v3;
    u_short pad26;
} POLY_FT4;

/* Textured sprite, 0x14 bytes. Built by SetSprt. */
typedef struct SPRT {
    P_TAG t;
    short x0;
    short y0;
    u_char u0;
    u_char v0;
    u_short clut;
    short w;
    short h;
} SPRT;

typedef struct SPRT_8 {
    P_TAG t;
    short x0;
    short y0;
    u_char u0;
    u_char v0;
    u_short clut;
} SPRT_8;

/* Solid rectangle, 0x10 bytes. Built by SetTile. */
typedef struct TILE {
    P_TAG t;
    short x0;
    short y0;
    short w;
    short h;
} TILE;

/* Flat line, 0x10 bytes. Built by SetLineF2. */
typedef struct LINE_F2 {
    P_TAG t;
    short x0;
    short y0;
    short x1;
    short y1;
} LINE_F2;

/* Flat 3-point polyline, 0x18 bytes. Built by SetLineF3. */
typedef struct LINE_F3 {
    P_TAG t;
    short x0;
    short y0;
    short x1;
    short y1;
    short x2;
    short y2;
    u_long pad14;
} LINE_F3;

/* Gradient line, 0x14 bytes. Built by SetLineG2. */
typedef struct LINE_G2 {
    P_TAG t;
    short x0;
    short y0;
    u_char r1;
    u_char g1;
    u_char b1;
    u_char pad0F;
    short x1;
    short y1;
} LINE_G2;

typedef struct DrawPacket {
    u_long tag;
    u_long len;
    u_long code[2];
} DrawPacket;

/*
 * The libgpu driver table at 0x800941A0 (g_GpuFuncs points at it). Slots
 * holding a `u_long` are worker
 * function addresses passed to `send` rather than called directly:
 *   +0x04 _addque        +0x08 Gpu_AddQueue      +0x0C Gpu_ClearImage
 *   +0x10 Gpu_WriteGp1   +0x14 Gpu_WriteGp0Words +0x18 Gpu_StartDmaTransfer
 *   +0x1C Gpu_StoreImage +0x20 Gpu_LoadImage     +0x24 Gpu_ExecuteQueue
 *   +0x28 Gpu_GetControlMirrorByte               +0x2C Gpu_ClearOTagDma
 *   +0x30 _param         +0x34 Gpu_Reset         +0x38 _status
 *   +0x3C Gpu_DrawSync
 */
typedef struct GpuCallbacks {
    u_char pad0[0x8];
    long (*send)(u_long worker, void *buf, long size, u_long data);
    u_long cmd0C;
    void (*submit)(long cmd);
    void (*writeGp0Words)(void *src, long count);
    u_long sendList;
    u_long storeImage;
    u_long loadImage;
    u_char pad24[0x28 - 0x24];
    long (*read)(long cmd);
    void (*clearOTag)(void *ot, long count);
    u_char pad30[0x34 - 0x30];
    void (*resetGraph)(long mode);
    long (*status)(void);
    void (*drawSync)(long mode);
} GpuCallbacks;

/*
 * The libgpu global env head at g_GpuFuncs. The same twelve bytes are spelled
 * as separate globals elsewhere (g_GpuFuncs, the printf hook, g_GraphType,
 * g_GraphQueue, g_GraphDebug, g_GraphReverse, g_VramWidth, g_VramHeight);
 * both spellings are load-bearing, because gcc 2.6.3 treats a struct member
 * reference as non-aliasing and the two forms schedule differently.
 */
typedef struct GfxState {
    GpuCallbacks *funcs;
    void (*printf)(char *, ...);
    u_char graphType;
    u_char graphQueue;
    volatile u_char graphDebug;
    volatile u_char graphReverse;
    short vramWidth;
    short vramHeight;
} GfxState;

/*
 * libgpu primitive initialisers. Each stamps the word count into prim[3] and
 * the GPU command byte into prim[7] (see the GP0 opcode in the comment).
 */
void SetPolyF3(void *prim);   /* 0x20 */
void SetPolyFT3(void *prim);  /* 0x24 */
void SetPolyG3(void *prim);   /* 0x30 */
void SetPolyGT3(void *prim);  /* 0x34 */
void SetPolyF4(void *prim);   /* 0x28 */
void SetPolyFT4(void *prim);  /* 0x2C */
void SetPolyG4(void *prim);   /* 0x38 */
void SetPolyGT4(void *prim);  /* 0x3C */
void SetSprt8(void *prim);    /* 0x74 */
void SetSprt16(void *prim);   /* 0x7C */
void SetSprt(void *prim);     /* 0x64 */
void SetTile1(void *prim);    /* 0x68 */
void SetTile8(void *prim);    /* 0x70 */
void SetTile16(void *prim);   /* 0x78 */
void SetTile(void *prim);     /* 0x60 */
void SetLineF2(void *prim);   /* 0x40 */
void SetLineG2(void *prim);   /* 0x50 */
void SetLineF3(void *prim);   /* 0x48 */
void SetLineG3(void *prim);   /* 0x58 */
void SetLineF4(void *prim);   /* 0x4C */
void SetLineG4(void *prim);   /* 0x5C */

/*
 * The same codes as constants, for the two the engine stamps itself rather than
 * going through the Set* helper: InitRenderState seeds g_RenderState.ft4Color[3] and
 * g_RenderState.gt4Color[3], the fourth byte of the two packed GTE RGBC words the
 * handwritten EmitPolyFT4Fog / EmitPolyGT4Fog emitters copy into each packet
 * (see game/render_state.h).
 */
#define POLY_FT4_CODE 0x2C
#define POLY_GT4_CODE 0x3C

/*
 * Bit 15 of a 15-bit BGR555 pixel, i.e. of a CLUT entry: the STP / mask bit.
 * The other fifteen bits are the colour, which is what the three field masks in
 * menu/menu_visual_effects.c isolate - 0xFFE0, 0xFC1F and 0x83FF each clear one
 * 5-bit channel and each leave bit 15 alone. A CLUT entry of 0x0000 is fully
 * transparent, so 0x8000 is the way to spell opaque black; StoreTeamLogoImage
 * puts it in entry 0 for the duration of one StoreImage and clears it after.
 */
#define CLUT_STP_BIT 0x8000

/* Primitive attribute bits (bit 1 = semi-transparency, bit 0 = shade-texture). */
void SetSemiTrans(void *prim, long enabled);
void SetShadeTex(void *prim, long enabled);

/* Ordering-table / primitive-list plumbing (24-bit "next" pointer in the tag). */
void SetPrimAddr(u_long *prim, u_long addr);
void TermPrim(u_long *prim);
long GetPrimAddr(u_long *prim);
void AddPrim(void *ot, void *prim);
void AddPrims(void *ot, void *first, void *last);
void *ClearOTag(u_long *ot, long count);
void *ClearOTagR(u_long *ot, long count);

/* Draw/display environment and texture-page helpers. */
/* CheckPrim is declared locally per TU: callers pass either a
 * Rect * or a GpuRectPacked *, and gcc 2.6.3 will not accept both against one
 * prototype. */
void ClearImage(void *rect, u_char r, u_char g, u_char b);
void LoadImage(Rect *rect, void *data);
void StoreImage(Rect *rect, void *data);
long MoveImage(GpuRectPacked *rect, u_long x, u_long y);
/* LibRef47 6-33 returns long (queue length for mode 1); no caller here uses the
 * result, so it is declared void. */
void DrawSync(long mode);
u_long DrawSyncCallback(u_long callback);
void DumpClut(long clut);
void DumpTPage(long tpage);
void DumpDrawEnv(DrawEnv *env);
void DumpDispEnv(DispEnv *env);
long GetClut(long x, long y);
long GetTPage(long tp, long abr, long x, long y);
DispEnv *GetDispEnv(DispEnv *env);
DrawEnv *GetDrawEnv(DrawEnv *env);
/* Draws one primitive immediately (DrawSync + push prim[4..] for prim[3]
 * words). */
void DrawPrim(u_char *prim);
/* Fills the 0x1C-byte DRAWENV head: clip, ofs, tw, tpage, dtd, dfe, isbg, rgb.
 * `dfe` comes from the buffer height and the DMA interrupt state. */
DrawEnv *SetDefDrawEnv(DrawEnv *env, long x, long y, long w, long h);
/* Fills the 0x14-byte DISPENV: disp Rect, screen Rect, isinter, isrgb24.
 * Was bound to SetDefDrawEnv here; that was wrong. */
DispEnv *SetDefDispEnv(DispEnv *env, long x, long y, long w, long h);
void SetDrawTPage(u_char *prim, long dfe, long dtd, long tpage);
void SetTexWindow(DrawPacket *prim, void *tw);
void SetDrawArea(DrawPacket *prim, Rect *rect);
void SetDrawOffset(DrawPacket *prim, short *ofs);
void SetDrawMode(
    DrawPacket *prim,
    long dfe,
    long dtd,
    long tpage,
    void *tw);
long LoadClut(void *clut, long x, long y);
long LoadClut2(void *clut, long x, long y);
/* g_GraphType (mode) and g_GraphDebug (debug level) accessors. */
long GetGraphType(void);
long GetGraphDebug(void);
/* GP1(03h) display enable: 0 blanks the screen (and clears the cached
 * DISPENV), non-zero shows it. Named from its own "SetDispMask(%d)..." trace
 * string at g_MsgGpuSetDispMask. */
void SetDispMask(long mask);

/* libgpu-internal byte fill helper. */
/* The body narrows `value` to u_char, but both callers were compiled against
 * a full word and pass -1; declaring it that way here keeps their code. */
void MemFill(u_char *dst, long value, long count);

/* Declared identically by 38 translation units before this
 * header carried them. */

/* Declared identically by 7 translation units before this
 * header carried them. */

long Gpu_ClearImage(short *rect, u_long rgb);
long Gpu_ClearOTagDma(u_long *ot, long count);
long Gpu_DrawSync(long mode);
u_char Gpu_GetControlMirrorByte(long index);
long Gpu_WriteGp0Words(u_long *src, long count);
void Gpu_WriteGp1(u_long command);
long SetGraphQueue(long mode);
long Gpu_ProbeType(u_long probe);
void Gpu_StartDmaTransfer();
u_long _param(u_long index);
long Gpu_AddQueue();
void Gpu_ArmTimeout(void);
long Gpu_CheckTimeout(void);
long Gpu_ExecuteQueue(void);
u_long _get_mode(long dfe, long dtd, u_long tpage);
u_long Gpu_BuildDrawAreaTopLeftCmd(long x, long y);
u_long Gpu_BuildDrawAreaBottomRightCmd(long x, long y);
u_long Gpu_BuildDrawOffsetCmd(long x, long y);
u_long Gpu_BuildTexWindowCmd(GpuTexWindow *window);
u_long get_dx();
void CheckPrim();

#endif
