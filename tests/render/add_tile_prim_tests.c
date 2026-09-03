#include "game/prim.h"
#include "game/render.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    GameOrderingTableEntry ot = {0};
    TILE tile;
    u8 *next;

    memset(&tile, 0, sizeof(tile));
    next = AddTilePrim(&ot, (u8 *)&tile, 11, 22, 33, 44, 55, 66, 77);

    CHECK(next == (u8 *)(&tile + 1));
    CHECK(tile.x0 == 11 && tile.y0 == 22);
    CHECK(tile.w == 33 && tile.h == 44);
    CHECK(tile.r0 == 55 && tile.g0 == 66 && tile.b0 == 77);
    CHECK(getlen(&tile) == 3 && getcode(&tile) == 0x60);

    memset(&tile, 0, sizeof(tile));
    next = GameQueueTileTrans(&ot, (u8 *)&tile, -1, -2, 30, 40,
                              50, 60, 70);
    CHECK(next == (u8 *)(&tile + 1));
    CHECK(tile.x0 == -1 && tile.y0 == -2);
    CHECK(tile.w == 30 && tile.h == 40);
    CHECK(tile.r0 == 50 && tile.g0 == 60 && tile.b0 == 70);
    CHECK(getlen(&tile) == 3 && getcode(&tile) == 0x62);

    puts("opaque and semi-transparent tile emitters passed");
    return 0;
}
