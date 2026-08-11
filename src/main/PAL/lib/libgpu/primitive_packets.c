#include "common.h"

typedef struct PrimTag {
#ifdef __psyz
    uintptr_t addr;
    uintptr_t len;
    u8 r0, g0, b0, code;
#else
    unsigned addr : 24;
    unsigned len : 8;
#endif
} PrimTag;

#ifdef __psyz
#define PRIM_SET_LEN(p, value) (((PrimTag *)(p))->len = (value))
#define PRIM_CODE(p) (((PrimTag *)(p))->code)
#define PRIM_DATA(p, psxOffset) ((u8 *)(p) + (psxOffset) + 12)
#else
#define PRIM_SET_LEN(p, value) (((PrimTag *)(p))->len = (value))
#define PRIM_CODE(p) (((u8 *)(p))[7])
#define PRIM_DATA(p, psxOffset) ((u8 *)(p) + (psxOffset))
#endif

s32 GetPrimAddr(u32 *p) {
    return (s32)(((PrimTag *)p)->addr | 0x80000000);
}

s32 IsEndPrim(u32 *p) {
    return ((PrimTag *)p)->addr == 0x00FFFFFF;
}

void AddPrim(void *ot, void *p) {
    PrimTag *otTag = (PrimTag *)ot;
    PrimTag *primTag = (PrimTag *)p;

    primTag->addr = otTag->addr;
    otTag->addr = (uintptr_t)primTag;
}

void AddPrims(void *ot, void *first, void *last) {
    PrimTag *otTag = (PrimTag *)ot;
    PrimTag *firstTag = (PrimTag *)first;
    PrimTag *lastTag = (PrimTag *)last;

    lastTag->addr = otTag->addr;
    otTag->addr = (uintptr_t)firstTag;
}

void SetPrimAddr(u32 *p, u32 addr) {
    ((PrimTag *)p)->addr = addr;
}

void TermPrim(u32 *p) {
    ((PrimTag *)p)->addr = 0x00FFFFFF;
}

void SetSemiTrans(u8 *p, s32 abe) {
    s32 value;

    if (abe != 0) {
        value = PRIM_CODE(p) | 2;
    } else {
        value = PRIM_CODE(p) & 0xFD;
    }
    PRIM_CODE(p) = value;
}

void SetShadeTex(u8 *p, s32 tge) {
    s32 value;

    if (tge != 0) {
        value = PRIM_CODE(p) | 1;
    } else {
        value = PRIM_CODE(p) & 0xFE;
    }
    PRIM_CODE(p) = value;
}

void SetPolyF3(u8 *p) {
    PRIM_SET_LEN(p, 4); PRIM_CODE(p) = 0x20;
}

void SetPolyFT3(u8 *p) {
    PRIM_SET_LEN(p, 7); PRIM_CODE(p) = 0x24;
}

void SetPolyG3(u8 *p) {
    PRIM_SET_LEN(p, 6); PRIM_CODE(p) = 0x30;
}

void SetPolyGT3(u8 *p) {
    PRIM_SET_LEN(p, 9); PRIM_CODE(p) = 0x34;
}

void SetPolyF4(u8 *p) {
    PRIM_SET_LEN(p, 5); PRIM_CODE(p) = 0x28;
}

void SetPolyFT4(u8 *p) {
    PRIM_SET_LEN(p, 9); PRIM_CODE(p) = 0x2C;
}

void SetPolyG4(u8 *p) {
    PRIM_SET_LEN(p, 8); PRIM_CODE(p) = 0x38;
}

void SetPolyGT4(u8 *p) {
    PRIM_SET_LEN(p, 0xC); PRIM_CODE(p) = 0x3C;
}

void SetSprt8(u8 *p) {
    PRIM_SET_LEN(p, 3); PRIM_CODE(p) = 0x74;
}

void SetSprt16(u8 *p) {
    PRIM_SET_LEN(p, 3); PRIM_CODE(p) = 0x7C;
}

void SetSprt(u8 *p) {
    PRIM_SET_LEN(p, 4); PRIM_CODE(p) = 0x64;
}

void SetTile1(u8 *p) {
    PRIM_SET_LEN(p, 2); PRIM_CODE(p) = 0x68;
}

void SetTile8(u8 *p) {
    PRIM_SET_LEN(p, 2); PRIM_CODE(p) = 0x70;
}

void SetTile16(u8 *p) {
    PRIM_SET_LEN(p, 2); PRIM_CODE(p) = 0x78;
}

void SetTile(u8 *p) {
    PRIM_SET_LEN(p, 3); PRIM_CODE(p) = 0x60;
}

void SetLineF2(u8 *p) {
    PRIM_SET_LEN(p, 3); PRIM_CODE(p) = 0x40;
}

void SetLineG2(u8 *p) {
    PRIM_SET_LEN(p, 4); PRIM_CODE(p) = 0x50;
}

void SetLineF3(u8 *p) {
    u32 value;

    value = 0x55555555;
    PRIM_SET_LEN(p, 5); PRIM_CODE(p) = 0x48;
    *(u32 *)PRIM_DATA(p, 0x14) = value;
}

void SetLineG3(u8 *p) {
    u32 value;

    value = 0x55555555;
    PRIM_SET_LEN(p, 7); PRIM_CODE(p) = 0x58;
    *(u32 *)PRIM_DATA(p, 0x1C) = value;
}

void SetLineF4(u8 *p) {
    u32 value;

    value = 0x55555555;
    PRIM_SET_LEN(p, 6); PRIM_CODE(p) = 0x4C;
    *(u32 *)PRIM_DATA(p, 0x18) = value;
}

void SetLineG4(u8 *p) {
    u32 value;

    value = 0x55555555;
    PRIM_SET_LEN(p, 9); PRIM_CODE(p) = 0x5C;
    *(u32 *)PRIM_DATA(p, 0x24) = value;
}

void SetDrawPacketTag(u8 *p) {
    PRIM_SET_LEN(p, 3); PRIM_CODE(p) = 2;
}

void SetDrawMove(u8 *p) {
    PRIM_SET_LEN(p, 5); PRIM_CODE(p) = 1;
    *(u32 *)PRIM_DATA(p, 8) = 0x80000000;
}
