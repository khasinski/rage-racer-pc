#include "common.h"
#include "game/menu.h"
#include "game/render.h"
#include "game/scratchpad.h"

s32 DrawClassChangeCurtain(s32 step) {
    void *scratch;
    s32 delta;
    register s32 value asm("$16");
    s32 y1;
    s32 red;
    s32 green;
    s32 blue;
    s32 alpha;
    s32 temp;
    register s32 zero asm("$5");
    register void *callScratch asm("$4");
    s32 yArg;
    u32 curtainPhase;

    scratch = SCRATCH_OT_BASE;
    delta = step;

    if (delta == 0) {
        g_ClassChangeCurtainSlide = 0;
        return g_ClassChangeCurtainSlide;
    } else {
        if (delta < 0) {
            temp = g_ClassChangeCurtainSlide;
            { s32 rel = temp; temp = delta + rel; }
            g_ClassChangeCurtainSlide = temp;
            if (temp < 0) {
                g_ClassChangeCurtainSlide = 0;
            }
        }

        value = g_ClassChangeCurtainSlide;
        if (value >= 0 && g_MenuAltLayout == 0) {
            if (value >= 0x10) {
                value = 0xF;
            }
            callScratch = scratch;
            zero = 0;
            asm("" : "=r"(zero) : "0"(zero));
            curtainPhase = value;
            value = ((curtainPhase << 9) / 32) + 0xFF10;
            yArg = (s16)value;
            y1 = 0xF0;
            red = 0x95;
            green = 0x25;
            blue = 0x1E;
            alpha = 0xFF;
            DrawSolidRect(callScratch, zero, yArg, 0x140, y1, red, green, blue, alpha);
            callScratch = scratch;
            zero = 0;
            asm("" : "=r"(zero) : "0"(zero));
            value = y1 - value;
            value <<= 0x10;
            yArg = value >> 0x10;
            DrawSolidRect(callScratch, zero, yArg, 0x140, y1, red, green, blue, alpha);
        }

        if (delta > 0) {
            temp = g_ClassChangeCurtainSlide;
            { s32 rel = temp; temp = delta + rel; }
            g_ClassChangeCurtainSlide = temp;
            if (temp >= 0x1A) {
                g_ClassChangeCurtainSlide = 0x19;
            }
        }
    }

    value = g_ClassChangeCurtainSlide;
    return value;
}
