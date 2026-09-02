#ifndef RAGE_PSYQ_PRESS_H
#define RAGE_PSYQ_PRESS_H

#include "common.h"

/* The libpress entry points used by the native FMV decoder. Keep these
 * declarations separate from the CD streaming API: they are implemented by
 * libpress and operate on ordinary host buffers. */
void DecDCTReset(int mode);
int DecDCTvlc(u_long *bitstream, u_long *runLevel);
void DecDCTin(u_long *runLevel, int mode);
void DecDCTout(u_long *pixels, int wordCount);

#endif
