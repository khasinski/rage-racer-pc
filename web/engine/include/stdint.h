#ifndef RAGE_PC_STDINT_H
#define RAGE_PC_STDINT_H

/* This directory is on the project's include path for the recovered game
 * headers.  Forward the standard header explicitly so host libc headers that
 * require int32_t keep working as well. */
#include_next <stdint.h>

#endif
