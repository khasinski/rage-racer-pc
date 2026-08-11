#ifndef RAGE_PORT_PSYQ_GTE_H
#define RAGE_PORT_PSYQ_GTE_H

#include <libgte.h>

typedef MATRIX Matrix;

MATRIX *MulMatrix2(MATRIX *left, MATRIX *right);
short *ApplyMatrixSV(void *matrix, void *input, short *output);
void Intpl(void *input, long blend, void *output);

#endif
