#ifndef RAGE_PORT_PSYQ_GTE_H
#define RAGE_PORT_PSYQ_GTE_H

#include <libgte.h>

typedef MATRIX Matrix;

MATRIX *MulMatrix2(MATRIX *left, MATRIX *right);
short *ApplyMatrixSV(const void *matrix, const void *input, short *output);
void Intpl(void *input, long blend, void *output);

static inline MATRIX *RageMulMatrix(void *left, void *right) {
    return MulMatrix((MATRIX *)left, (MATRIX *)right);
}
static inline MATRIX *RageMulMatrix2(void *left, void *right) {
    return MulMatrix2((MATRIX *)left, (MATRIX *)right);
}
static inline void RageApplyMatrix(void *matrix, void *input, void *output) {
    ApplyMatrix((MATRIX *)matrix, (SVECTOR *)input, (VECTOR *)output);
}
static inline MATRIX *RageScaleMatrix(void *matrix, void *scale) {
    return ScaleMatrix((MATRIX *)matrix, (VECTOR *)scale);
}
static inline MATRIX *RageRotMatrix(void *rotation, void *matrix) {
    return RotMatrix((SVECTOR *)rotation, (MATRIX *)matrix);
}

#define MulMatrix RageMulMatrix
#define MulMatrix2 RageMulMatrix2
#define ApplyMatrix RageApplyMatrix
#define ScaleMatrix RageScaleMatrix
#define RotMatrix RageRotMatrix

#endif
