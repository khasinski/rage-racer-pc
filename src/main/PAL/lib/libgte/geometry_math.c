#include "common.h"
#include "psyq/gte_macros.h"

/*
 * GTE COP2 leaf routines after reg03: the fog/colour control writers
 * (SetBackColor/SetFarColor/SetGeomOffset/SetGeomScreen, reg04.o family,
 * SetBackColor..) and the single-command colour/vector ops (LightColor,
 * DpqColor, DpqColor3, Intpl, Square12/0, AverageZ3/4, OuterProduct12/0 =
 * smp_00.o).  Kept as one residual TU: the smp_00.o boundary at 0x80069A88
 * sits under load-bearing `.align 4` padding (func_80069A70/A84) that cannot
 * be split off without disturbing byte-exactness, so smp_00.o is NOT carved
 * out here. 
 */

void SetBackColor(s32 a, s32 b, s32 c) {
    s32 x = a * 16, y = b * 16, z = c * 16;
    gte_ctc2(x, 13);
    gte_ctc2(y, 14);
    gte_ctc2(z, 15);
}
void SetFarColor(s32 a, s32 b, s32 c) {
    s32 x = a * 16, y = b * 16, z = c * 16;
    gte_ctc2(x, 21);
    gte_ctc2(y, 22);
    gte_ctc2(z, 23);
}
void SetGeomOffset(s32 a, s32 b) {
    s32 x = a << 16, y = b << 16;
    gte_ctc2(x, 24);
    gte_ctc2(y, 25);
}
__asm__(".align 4");
void SetGeomScreen(s32 a) { gte_ctc2(a, 26); }

/* --- LightColor.s .. OuterProduct0.s --- */

__asm__(".align 4");
void LightColor(void *in, void *out) {
    gte_ldir(in);
    gte_nop();
    gte_lc();
    gte_stir(out);
}

void DpqColor(void *in, void *rgb, s32 ir0, void *out) {
    gte_ldir(in);
    gte_lwc2(6, rgb);
    gte_mtc2(ir0, 8);
    gte_nop();
    gte_dpcl();
    gte_swc2(22, out);
}

void DpqColor3(void *v0, void *v1, void *v2, s32 ir0, void *o0, void *o1,
                   void *o2) {
    (void)o0;
    (void)o1;
    (void)o2;
    gte_dpct3(v0, v1, v2, ir0);
}

void Intpl(void *in, s32 ir0, void *out) {
    gte_ldir(in);
    gte_mtc2(ir0, 8);
    gte_nop();
    gte_intpl();
    gte_swc2(22, out);
}

void *Square12(void *in, void *out) {
    register void *p asm("$5") = out;
    gte_ldir(in);
    gte_nop();
    gte_sqr12();
    gte_stmac(p);
    return p;
}

void *Square0(void *in, void *out) {
    register void *p asm("$5") = out;
    gte_ldir(in);
    gte_nop();
    gte_sqr0();
    gte_stmac(p);
    return p;
}

s32 AverageZ3(s32 a, s32 b, s32 c) {
    s32 r;
    gte_mtc2(a, 17);
    gte_mtc2(b, 18);
    gte_mtc2(c, 19);
    gte_nop();
    gte_avsz3();
    gte_mfc2(r, 7);
    return r;
}

s32 AverageZ4(s32 a, s32 b, s32 c, s32 d) {
    s32 r;
    gte_mtc2(a, 16);
    gte_mtc2(b, 17);
    gte_mtc2(c, 18);
    gte_mtc2(d, 19);
    gte_nop();
    gte_avsz4();
    gte_mfc2(r, 7);
    return r;
}

void OuterProduct12(void *m, void *v, void *out) {
    gte_op12_diag(m, v, out);
}

void OuterProduct0(void *m, void *v, void *out) {
    gte_op0_diag(m, v, out);
}
