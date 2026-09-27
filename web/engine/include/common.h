#ifndef RAGE_PC_COMMON_H
#define RAGE_PC_COMMON_H

typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef float f32;

#include <stdint.h>

/*
 * Reads or writes a struct member without the compiler's "this lives inside a
 * struct" mark. That mark makes gcc 2.6.3 assume the access cannot alias a
 * plain global, which lets it move a neighbouring global store across the
 * access; the retail code keeps the two in source order. Going through an
 * integer round-trip defeats the fold back to a member reference, so the
 * access is just a load or store at a computed address, and the ordering is
 * restored without a memory clobber. Same address, same width, no barrier.
 */
#define RETAIL_ORDERED_ACCESS(x) (*(__typeof__(x) *)(uintptr_t)&(x))

#endif
