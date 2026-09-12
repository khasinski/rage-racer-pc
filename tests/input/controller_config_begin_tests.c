#include "game/input_internal.h"

#include <stdio.h>

ControllerMappingIndex g_PadMappingIndex;
ControllerMappingIndex g_NegconMappingIndex;
ControllerSetup g_ControllerSetup;

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
    g_ControllerSetup.angleX = 1;
    g_ControllerSetup.angleY = 2;

    BeginControllerConfig();

    CHECK(g_PadMappingIndex == CONTROLLER_MAPPING_FIRST);
    CHECK(g_NegconMappingIndex == CONTROLLER_MAPPING_LAST);
    CHECK(g_ControllerSetup.savedPadMapping == CONTROLLER_MAPPING_FIRST);
    CHECK(g_ControllerSetup.savedNegconMapping == CONTROLLER_MAPPING_LAST);
    CHECK(g_ControllerSetup.angleX == 0 && g_ControllerSetup.angleY == 0);

    puts("controller config begins from normalized mapping selections");
    return 0;
}
