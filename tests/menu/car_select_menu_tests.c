#include "common.h"
#include "game/asset.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/render_state.h"

#include <stdio.h>

s32 g_CarSelectFadeAccum;
s32 g_PlayerCarIndex;
static CarEntry s_cars[GAME_CAR_COUNT];
CarEntry *g_CarTable = s_cars;
static CarModelAsset s_model;
CarModelAsset *g_CarModelAsset = &s_model;
GameRenderState g_RenderState;

typedef struct SpriteRecord {
    s32 x;
    s32 textureU;
    u8 brightness;
} SpriteRecord;

static SpriteRecord s_sprites[2];
static s32 s_spriteCount;
static s32 s_outlineCount;
static u8 s_outlineBrightness;

void DrawRectOutline(void *ot, s32 x, s32 y, s32 width, s32 height, u8 r,
                     u8 g, u8 b, u8 flags) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)g;
    (void)b;
    (void)flags;
    s_outlineCount++;
    s_outlineBrightness = r;
}

void DrawSprite(GameOrderingTableEntry *ot, s16 x, s16 y, s16 width, u16 height, u16 textureU,
                u16 textureV, u8 r, u8 g, u8 b, u16 clut, s32 shadeTex,
                s32 semiTrans, u32 flags) {
    (void)ot;
    (void)y;
    (void)width;
    (void)height;
    (void)textureV;
    (void)g;
    (void)b;
    (void)clut;
    (void)shadeTex;
    (void)semiTrans;
    (void)flags;
    s_sprites[s_spriteCount++] = (SpriteRecord){x, textureU, r};
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,         \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    g_CarSelectFadeAccum = 100;
    CHECK(DrawCarSelectScreen(0) == 0);
    CHECK(s_outlineCount == 0 && s_spriteCount == 0);

    s_cars[0].transmission = 1;
    s_model.gearCount = 5;
    CHECK(DrawCarSelectScreen(600) == MENU_FADE_MAX);
    CHECK(s_outlineCount == 1 && s_outlineBrightness == MENU_FADE_MAX / 4);
    CHECK(s_spriteCount == 2);
    CHECK(s_sprites[0].x == 0xAD && s_sprites[0].textureU == 0x6C);
    CHECK(s_sprites[1].x == 0xA5 && s_sprites[1].textureU == 0x28);

    s_spriteCount = 0;
    s_cars[0].transmission = 0;
    s_model.gearCount = 6;
    CHECK(DrawCarSelectScreen(-MENU_FADE_MAX) == 0);
    CHECK(s_sprites[0].x == 0xAE && s_sprites[0].textureU == 0x60);
    CHECK(s_sprites[1].x == 0xA6 && s_sprites[1].textureU == 0x30);
    CHECK(s_sprites[1].brightness == 0);

    s_spriteCount = 0;
    s_model.gearCount = 3;
    DrawCarSelectScreen(1);
    CHECK(s_spriteCount == 1);

    puts("car select menu tests passed");
    return 0;
}
