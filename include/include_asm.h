#ifndef RAGE_PC_INCLUDE_ASM_H
#define RAGE_PC_INCLUDE_ASM_H

#if !defined(M2CTX) && !defined(PERMUTER) && !defined(__psyz) && \
    !defined(RAGE_HOST_PORT)

/* A block the original shipped as assembly, sitting inside a unit that is
   otherwise C: a kernel entry reached by `syscall`, a BIOS call that jumps
   through a register, a GTE routine that moves coprocessor control registers.
   The assembly lives in a .s beside the source and is pulled in here so that
   it lands at the right offset within the unit.

   A unit that is assembly end to end needs none of this. It is a .s in its own
   right and assembled directly by CMake. This macro exists only for the mixed
   case, where the assembly has
   to be interleaved with compiled C.

   The block is wrapped in a throwaway function because the compiler will not
   carry a file-scope `.include` through maspsx untouched. maspsx drops the
   body and leaves the name behind as an undefined reference, which is harmless
   in the image. */
#ifndef HANDWRITTEN_ASM
#define HANDWRITTEN_ASM(FOLDER, NAME) \
    void __maspsx_include_asm_hack_##NAME(void) { \
        __asm__( \
            ".text # maspsx-keep\n" \
            "\t.align\t2 # maspsx-keep\n" \
            "\t.set\tnoreorder # maspsx-keep\n" \
            "\t.set\tnoat # maspsx-keep\n" \
            "\t.include \"" FOLDER "/" #NAME ".s\" # maspsx-keep\n" \
            "\t.set\treorder # maspsx-keep\n" \
            "\t.set\tat # maspsx-keep\n" \
        ); \
    }
#endif

__asm__(".include \"include/macro.inc\"\n");

#else

#ifndef HANDWRITTEN_ASM
#define HANDWRITTEN_ASM(FOLDER, NAME)
#endif

#endif

#endif
