#include "common.h"
#include "psyq/gte_macros.h"

/*
 * PSY-Q 3.5 libgte object reg03.o (LIBGTE.A): the direct GTE data-register
 * writers, SetVertex0..SetDQB.  Exports SetVertex0/1/2, SetVertexTri,
 * SetRGBfifo, SetIR123, SetIR0, SetSZfifo3/4, SetSXSYfifo, SetRii, SetMAC123,
 * SetData32, SetDQA, SetDQB.  Boundaries byte-matched against reg03.o
 *.  Bodies use the named GTE macros.
 */

void SetVertex0(void *v) { gte_ldv0(v); }
void SetVertex1(void *v) { gte_ldv1(v); }
void SetVertex2(void *v) { gte_ldv2(v); }
void SetVertexTri(void *v0, void *v1, void *v2) {
    gte_ldv0(v0);
    gte_ldv1(v1);
    gte_ldv2(v2);
}

/* --- SetRGBfifo.s --- */

void SetRGBfifo(void *a, void *b, void *c) {
    gte_lwc2(20, a);
    gte_lwc2(21, b);
    gte_lwc2(22, c);
}
void SetIR123(s32 a, s32 b, s32 c) {
    gte_mtc2(a, 9);
    gte_mtc2(b, 10);
    gte_mtc2(c, 11);
}
void SetIR0(s32 a) { gte_mtc2(a, 8); }
void SetSZfifo3(s32 a, s32 b, s32 c) {
    gte_mtc2(a, 17);
    gte_mtc2(b, 18);
    gte_mtc2(c, 19);
}
void SetSZfifo4(s32 a, s32 b, s32 c, s32 d) {
    gte_mtc2(a, 16);
    gte_mtc2(b, 17);
    gte_mtc2(c, 18);
    gte_mtc2(d, 19);
}
void SetSXSYfifo(s32 a, s32 b, s32 c) {
    gte_mtc2(a, 12);
    gte_mtc2(b, 13);
    gte_mtc2(c, 14);
}
void SetRii(s32 a, s32 b, s32 c) {
    gte_ctc2(a, 0);
    gte_ctc2(b, 2);
    gte_ctc2(c, 4);
}
void SetMAC123(s32 a, s32 b, s32 c) {
    gte_mtc2(a, 25);
    gte_mtc2(b, 26);
    gte_mtc2(c, 27);
}
void SetData32(s32 a) { gte_mtc2(a, 30); }
void SetDQA(s32 a) { gte_ctc2(a, 27); }
void SetDQB(s32 a) { gte_ctc2(a, 28); }
