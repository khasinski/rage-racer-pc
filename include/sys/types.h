#ifndef RAGE_PC_SYS_TYPES_H
#define RAGE_PC_SYS_TYPES_H

#ifdef RAGE_HOST_PORT
/* Host libc and SDL headers reach <sys/types.h> through this shadow; forward
 * to the real header first so glibc keeps int32_t and the UCRT keeps _dev_t
 * for the internals that include us indirectly. */
#include_next <sys/types.h>
#endif

/*
 * The unsigned type names every PSY-Q library header pulls in through
 * <sys/types.h>. They are what the Run-Time Library Reference spells its
 * signatures with, so the SDK code in this repo uses them instead of the
 * game's own s8/u8/.../u32 typedefs from common.h.
 */
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
#ifdef _WIN64
typedef unsigned long long u_long;
#else
typedef unsigned long u_long;
#endif

#endif
