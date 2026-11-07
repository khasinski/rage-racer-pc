#ifndef RAGE_PC_PSYQ_GTE_H
#define RAGE_PC_PSYQ_GTE_H

#include <sys/types.h>

#include "common.h"

typedef struct Matrix {
    short m[3][3];
    s32 t[3];
} Matrix;

_Static_assert(sizeof(Matrix) == 32, "PSY-Q Matrix must retain its 32-bit ABI");

/*
 * libgte leaf routines. These load GTE (COP2) control/data registers or run a
 * single COP2 command; the bodies live in render/SetRotMatrix.c and
 * are written with the gte_* macros from psyq/gte_macros.h.
 */
void SetRotMatrix(void *m);
void SetLightMatrix(void *m);
void SetColorMatrix(void *m);
void SetTransMatrix(void *m);
/* Direct GTE register writers (libgte REG03_1 through REG03_13). */
void SetVertex0(void *v);
void SetVertex1(void *v);
void SetVertex2(void *v);
void SetVertexTri(void *v0, void *v1, void *v2);
void SetRGBfifo(void *rgb0, void *rgb1, void *rgb2);
void SetIR123(long ir1, long ir2, long ir3);
void SetIR0(long ir0);
void SetSZfifo3(long sz1, long sz2, long sz3);
void SetSZfifo4(long sz0, long sz1, long sz2, long sz3);
void SetSXSYfifo(long sxy0, long sxy1, long sxy2);
void SetRii(long r11, long r22, long r33);
void SetMAC123(long mac1, long mac2, long mac3);
void SetData32(long data);
/* Perspective divide constants (control regs 27 / 28). */
void SetDQA(long dqa);
void SetDQB(long dqb);
/* Fog: derives dqa from the near distance `a` and projection distance `h`. */
void SetFogNear(long a, long h);
/* Background / far colours; each component is shifted left by 4 on the way in. */
void SetBackColor(long rbk, long gbk, long bbk);
void SetFarColor(long rfc, long gfc, long bfc);
void SetGeomOffset(long ofx, long ofy);
void SetGeomScreen(long h);
/* Single COP2 commands. */
void LightColor(void *v0, void *v1);
void DpqColor3(
    void *v0,
    void *v1,
    void *v2,
    long p,
    void *o0,
    void *o1,
    void *o2);
void DpqColor(void *v0, void *rgb, long p, void *v1);
void Intpl(void *v0, long p, void *v1);
void *Square12(void *v0, void *v1);
void *Square0(void *v0, void *v1);
long AverageZ3(long sz0, long sz1, long sz2);
long AverageZ4(long sz0, long sz1, long sz2, long sz3);
void OuterProduct12(void *v0, void *v1, void *v2);
void OuterProduct0(void *v0, void *v1, void *v2);
long NormalClip(long sxy0, long sxy1, long sxy2);
long Lzc(long data);
/* Transposes the 3x3 rotation part of m0 into m1, leaving the translation. */
Matrix *TransposeMatrix(Matrix *m0, Matrix *m1);

/*
 * Matrix multiply family. Each loads m0 into the GTE rotation control registers
 * (ctc2 $0..$4) and pushes the three columns of m1 through MVMVA
 * (cop2 0x486012, mx=rotation, v=V0, no translation, sf=12), so only the 3x3
 * rotation part is touched. They differ only in where the product lands:
 * MulMatrix  -> m0, MulMatrix2 -> m1, MulMatrix0 -> m2. Each returns it.
 */
void *MulMatrix(void *m0, void *m1);
void *MulMatrix2(void *m0, void *m1);
void *MulMatrix0(void *m0, void *m1, void *m2);
/* v1 = m * v0 through the same MVMVA; v0 is a short vector, v1 gets MAC1..3. */
void *ApplyMatrix(void *m, void *v0, void *v1);
/* SVECTOR in, SVECTOR out; returns v1. LibRef47 8-23. */
short *ApplyMatrixSV(const void *m, const void *v0, short *v1);

/* Matrix scaling. ScaleMatrix does m[i][j] *= v[j] (column j scaled by
 * v[j]); ScaleMatrixL does m[i][j] *= v[i]. LibRef47 8-150 / 8-151. */
void *ScaleMatrix(void *m, void *v);
void *ScaleMatrixL(void *m, void *v);
/* Rotation matrix from an SVECTOR of Z/Y/X Euler angles (raw asm sibling). */
void *RotMatrix(void *r, void *m);
/* Square root in 12-bit fixed point: returns sqrt(a << 12), i.e. 64*sqrt(a).
 * Normalises with Lzc, then runs the hyperbolic CORDIC in CordicRotate. */
long SquareRoot12(long a);

/* Declared identically by 22 translation units before this
 * header carried them. */

void EmitCoursePolyF4(void);
void EmitCoursePolyF4Fog(void);
void EmitCoursePolyFT4(void);
void EmitCoursePolyFT4Fog(void);
void EmitCourseSubdividedFT4(void);
void EmitCourseSubdividedFT4Fog(void);
void EmitPolyF4(void);
void EmitPolyF4Fog(void);
void EmitPolyFT4Fog(void);
void EmitPolyFT4Raw(void);
void EmitPolyG4(void);
void EmitPolyG4Fog(void);
void EmitPolyGT4(void);
void EmitPolyGT4Fog(void);
void EmitSubdividedTerrainQuad(void);
void InterpolateSubdivRow(void);
void SubmitCourseSubdividedFaces(void);
void SubmitModel2(void *ctx, s32 model);
void SubmitModelFaces(void);
void SubmitTerrainCellFaces(void);
void SubmitTerrainCellFacesFar(void);
void TransformCourseModel(void);

#endif
