#include "game/input_internal.h"

#include <stdio.h>

ControllerMappingIndex g_PadMappingIndex;
ControllerMappingIndex g_NegconMappingIndex;
static ControllerSetup s_controllerSetup;
ControllerSetup *MenuControllerSetup(void) { return &s_controllerSetup; }

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
    MenuControllerSetup()->angleX = 1;
    MenuControllerSetup()->angleY = 2;

    BeginControllerConfig(&s_controllerSetup);

    CHECK(g_PadMappingIndex == CONTROLLER_MAPPING_FIRST);
    CHECK(g_NegconMappingIndex == CONTROLLER_MAPPING_LAST);
    CHECK(MenuControllerSetup()->savedPadMapping == CONTROLLER_MAPPING_FIRST);
    CHECK(MenuControllerSetup()->savedNegconMapping == CONTROLLER_MAPPING_LAST);
    CHECK(MenuControllerSetup()->angleX == 0 && MenuControllerSetup()->angleY == 0);

    puts("controller config begins from normalized mapping selections");
    return 0;
}
