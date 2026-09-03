#include "common.h"
#include "game/car.h"
#include "game/menu.h"

#include <limits.h>
#include <stdio.h>

s32 g_CarNamePlateFade;
GameRenderState g_RenderState;

typedef struct DrawnSprite {
    s16 x;
    s16 y;
    s16 width;
    u16 textureU;
    u16 textureV;
    u8 shade;
    u32 flags;
} DrawnSprite;

static DrawnSprite s_draws[4];
static s32 s_drawCount;

s32 GetCarUnlockLevel(s32 model) { return model; }

void DrawSprite(GameOrderingTableEntry *ot, s16 x, s16 y, s16 width,
                u16 height, u16 u, u16 v, u8 r, u8 g, u8 b, u16 clut,
                s32 shadeTex, s32 semiTrans, u32 flags) {
    (void)ot;
    (void)height;
    (void)g;
    (void)b;
    (void)clut;
    (void)shadeTex;
    (void)semiTrans;
    if (s_drawCount < 4) {
        s_draws[s_drawCount] = (DrawnSprite){x, y, width, u, v, r, flags};
    }
    s_drawCount++;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,       \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static int CheckSprite(const DrawnSprite *sprite, s16 x, s16 y, s16 width,
                       u16 u, u16 v, u32 flags) {
    CHECK(sprite->x == x && sprite->y == y && sprite->width == width);
    CHECK(sprite->textureU == u && sprite->textureV == v);
    CHECK(sprite->shade == 127 && sprite->flags == flags);
    return 0;
}

int main(void) {
    static const DrawnSprite expectedManufacturers[GAME_CAR_COUNT] = {
        {0x112, 0, 0x14, 0x50, 0xBC, 0, 0},
        {0x112, 0, 0x14, 0x50, 0xBC, 0, 0},
        {0x112, 0, 0x14, 0x50, 0xBC, 0, 0},
        {0x105, 0, 0x20, 0x00, 0xBC, 0, 0},
        {0x106, 0, 0x20, 0x64, 0xBC, 0, 0},
        {0x106, 0, 0x20, 0x64, 0xBC, 0, 0},
        {0x106, 0, 0x20, 0x64, 0xBC, 0, 0},
        {0x0F6, 0, 0x30, 0x20, 0xBC, 0, 0},
        {0x0F6, 0, 0x30, 0x20, 0xBC, 0, 0},
        {0x0F6, 0, 0x30, 0x20, 0xBC, 0, 0},
        {0x112, 0, 0x14, 0x50, 0xBC, 0, 0},
        {0x106, 0, 0x20, 0x64, 0xBC, 0, 0},
        {0x0F6, 0, 0x30, 0x20, 0xBC, 0, 0},
    };
    static const DrawnSprite expectedNames[GAME_CAR_COUNT] = {
        {0x0FC, 0, 0x2A, 0x0A, 0x30, 0, 0},
        {0x106, 0, 0x20, 0x48, 0x30, 0, 0},
        {0x106, 0, 0x20, 0x7C, 0x30, 0, 0},
        {0x0F2, 0, 0x34, 0x00, 0x40, 0, 0},
        {0x0FD, 0, 0x28, 0x74, 0x50, 0, 0},
        {0x0FC, 0, 0x2A, 0x3E, 0x50, 0, 0},
        {0x107, 0, 0x20, 0xB0, 0x50, 0, 0},
        {0x0FE, 0, 0x28, 0x40, 0x40, 0, 0},
        {0x104, 0, 0x22, 0x7A, 0x40, 0, 0},
        {0x0F7, 0, 0x30, 0xA0, 0x40, 0, 0},
        {0x0FA, 0, 0x2C, 0xA4, 0x30, 0, 0},
        {0x0FC, 0, 0x2A, 0x0A, 0x60, 0, 0},
        {0x0F6, 0, 0x30, 0x04, 0x50, 0, 0},
    };
    s32 model;

    g_CarNamePlateFade = 99;
    s_drawCount = 0;
    DrawCarNamePlate(0, 0, 0);
    CHECK(g_CarNamePlateFade == 0 && s_drawCount == 0);

    for (model = 0; model < GAME_CAR_COUNT; model++) {
        const DrawnSprite *manufacturer = &expectedManufacturers[model];
        const DrawnSprite *name = &expectedNames[model];

        g_CarNamePlateFade = 508;
        s_drawCount = 0;
        DrawCarNamePlate(1, model, 0);
        CHECK(s_drawCount == 4 && g_CarNamePlateFade == 508);
        if (CheckSprite(&s_draws[2], manufacturer->x, 0x178,
                        manufacturer->width, manufacturer->textureU,
                        manufacturer->textureV, 0x3B)) return 1;
        if (CheckSprite(&s_draws[3], name->x, 0x188, name->width,
                        name->textureU, name->textureV, 0x3E)) return 1;
    }

    g_CarNamePlateFade = 8;
    s_drawCount = 0;
    DrawCarNamePlate(-20, -1, 0);
    CHECK(g_CarNamePlateFade == 0 && s_drawCount == 2);
    CHECK(s_draws[0].shade == 0 && s_draws[1].shade == 0);

    g_CarNamePlateFade = 1;
    DrawCarNamePlate(INT_MIN, 0, INT_MAX);
    CHECK(g_CarNamePlateFade == 0);
    DrawCarNamePlate(INT_MAX, 0, INT_MAX);
    CHECK(g_CarNamePlateFade == 508);

    puts("car name plate tests passed");
    return 0;
}
