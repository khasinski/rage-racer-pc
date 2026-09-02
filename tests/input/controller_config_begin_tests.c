#include "game/input_internal.h"

#include <stdio.h>

ControllerMappingIndex g_PadMappingIndex;
ControllerMappingIndex g_NegconMappingIndex;
u16 g_PadMappingIndexSaved;
u16 g_NegconMappingIndexSaved;
s32 g_ControllerSceneAngleX;
s32 g_ControllerSceneAngleY;
s32 g_PadConfigFlipPhase;
s32 g_PadConfigFlipTimer;

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    g_PadMappingIndex = -4;
    g_NegconMappingIndex = 20;
    g_ControllerSceneAngleX = 1;
    g_ControllerSceneAngleY = 2;
    g_PadConfigFlipPhase = 3;
    g_PadConfigFlipTimer = 4;

    BeginControllerConfig();

    CHECK(g_PadMappingIndex == CONTROLLER_MAPPING_FIRST);
    CHECK(g_NegconMappingIndex == CONTROLLER_MAPPING_LAST);
    CHECK(g_PadMappingIndexSaved == CONTROLLER_MAPPING_FIRST);
    CHECK(g_NegconMappingIndexSaved == CONTROLLER_MAPPING_LAST);
    CHECK(g_ControllerSceneAngleX == 0 && g_ControllerSceneAngleY == 0);
    CHECK(g_PadConfigFlipPhase == 0 && g_PadConfigFlipTimer == 0);

    puts("controller config begins from normalized mapping selections");
    return 0;
}
