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

static int CheckSprite(s32 semiTrans, u8 expectedCode) {
    const GameSpriteDesc desc = {
        .x = 123,
        .y = 45,
        .w = 67,
        .h = 89,
        .u0 = 17,
        .v0 = 29,
        .clut = 0x456,
        .semiTrans = semiTrans,
    };
    SPRT sprite;

    memset(&sprite, 0xA5, sizeof(sprite));
    BuildSpriteFromDesc(&sprite, &desc);

    CHECK(sprite.x0 == 123 && sprite.y0 == 45);
    CHECK(sprite.w == 67 && sprite.h == 89);
    CHECK(sprite.u0 == 17 && sprite.v0 == 29);
    CHECK(sprite.clut == 0x456);
    CHECK(sprite.code == expectedCode);
    return 0;
}

int main(void) {
    CHECK(CheckSprite(0, 0x65) == 0);
    CHECK(CheckSprite(1, 0x67) == 0);

    puts("sprite descriptions produce complete GPU packets");
    return 0;
}
