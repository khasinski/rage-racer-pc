#include "game/car.h"
#include "game/render.h"

#include <stdio.h>
#include <string.h>

GameCarRuntime g_Cars[11];

static s32 s_selectedBank;
static GameCarRuntime *s_drawn[11];
static s32 s_drawCount;

void SelectModelBank(s32 index) {
    s_selectedBank = index;
}

void DrawCar(GameRenderObject *object) {
    s_drawn[s_drawCount++] = (GameCarRuntime *)(void *)object;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    memset(g_Cars, 0, sizeof(g_Cars));
    g_Cars[1].activeFlag = -1;
    g_Cars[2].aiEnabled = 1;
    g_Cars[5].aiEnabled = 1;

    DrawCars();
    CHECK(s_selectedBank == 1 && s_drawCount == 2);
    CHECK(s_drawn[0] == &g_Cars[2] && s_drawn[1] == &g_Cars[5]);

    s_drawCount = 0;
    s_selectedBank = -1;
    DrawPlayerCarOnly();
    CHECK(s_selectedBank == 1 && s_drawCount == 1);
    CHECK(s_drawn[0] == &g_Cars[0]);

    puts("car drawing dispatch tests passed");
    return 0;
}
