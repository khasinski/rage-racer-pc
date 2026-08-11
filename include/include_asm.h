#ifndef RAGE_PC_INCLUDE_ASM_H
#define RAGE_PC_INCLUDE_ASM_H

#if !defined(M2CTX) && !defined(PERMUTER) && !defined(__psyz)

#ifndef INCLUDE_ASM_USE_MACRO_INC
#define INCLUDE_ASM_USE_MACRO_INC 1
#endif

#ifndef INCLUDE_ASM
#define INCLUDE_ASM(FOLDER, NAME) \
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

#ifndef INCLUDE_RODATA
#define INCLUDE_RODATA(FOLDER, NAME) \
    __asm__( \
        ".section .rodata\n" \
        "    .include \"" FOLDER "/" #NAME ".s\"\n" \
        ".section .text" \
    )
#endif

#if INCLUDE_ASM_USE_MACRO_INC
__asm__(".include \"include/macro.inc\"\n");
#else
__asm__(".include \"include/labels.inc\"\n");
#endif

#else

#ifndef INCLUDE_ASM
#define INCLUDE_ASM(FOLDER, NAME)
#endif

#ifndef INCLUDE_RODATA
#define INCLUDE_RODATA(FOLDER, NAME)
#endif

#endif

#endif
