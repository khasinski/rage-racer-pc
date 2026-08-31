#ifndef GAME_ENVIRONMENT_H
#define GAME_ENVIRONMENT_H

#include <stddef.h>

#include "common.h"

/*
 * These lay out retail's own bytes, so their packing is part of the format
 * rather than a preference. The attribute below is what keeps GameEnvColor
 * one-byte aligned, which is what puts the colour slots two bytes in, right
 * behind a s16. A toolchain that does not honour it starts them four bytes in
 * instead and every colour is then read two bytes out: red comes back where
 * blue was written. The pragma says the same thing to compilers targeting the
 * Microsoft ABI, which is what the Windows build uses, and the assertions at
 * the end refuse to build rather than let it happen quietly.
 */
#ifdef _MSC_VER
#pragma pack(push, 1)
#endif

typedef union GameEnvColor {
    struct {
        u32 rgb __attribute__((packed));
    } word;
    struct {
        u8 r;
        u8 g;
        u8 b;
        u8 unused;
    } bytes;
} GameEnvColor;

struct GameEnvironmentCue {
    s32 time;
    GameEnvColor colors[9];
    u16 duration;
    u16 reserved2A;
    u16 mode;
    u16 spareTarget;
};

typedef union GameEnvironmentScriptAddress {
    u32 *words;
    struct GameEnvironmentCue *cues;
} GameEnvironmentScriptAddress;

typedef struct GameEnvColorSlot {
    GameEnvColor cur;
    GameEnvColor from;
    GameEnvColor to;
} GameEnvColorSlot;

typedef union GameEnvironmentColors {
    struct {
        s16 fogEnabled;
        GameEnvColorSlot slots[9];
    } fields;
    u32 fogColorWord;
} GameEnvironmentColors;

#ifdef _MSC_VER
#pragma pack(pop)
#endif

_Static_assert(sizeof(GameEnvColor) == 4, "environment colour ABI changed");
_Static_assert(sizeof(GameEnvColorSlot) == 12, "environment slot ABI changed");
_Static_assert(offsetof(GameEnvironmentColors, fields.slots) == 2,
               "the colour slots must follow the fog flag with no padding, "
               "or every colour is read two bytes out and blue arrives as red");
/* The union's own size is left out on purpose: it shares storage with a u32,
 * so how far it rounds up is the compiler's business. What must not move is
 * where the slots start and how big each one is. */

extern GameEnvironmentColors g_EnvironmentColors;

#endif
