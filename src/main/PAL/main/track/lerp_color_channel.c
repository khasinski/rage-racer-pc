#include "common.h"

s32 LerpColorChannel(s32 from, s32 to, s32 blend) {
    return from + (((to - from) * blend) >> 12);
}
