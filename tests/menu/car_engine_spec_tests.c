#include "common.h"
#include "game/asset.h"
#include "game/menu.h"
#include "game/render_state.h"

#include <stdio.h>
#include <string.h>

s32 g_MenuAltLayout;
const char g_FormatDecimal[4] = "%d";
static CarModelAsset s_model;
CarModelAsset *g_CarModelAsset = &s_model;
GameRenderState g_RenderState;

typedef struct DrawRecord {
    char kind;
    s32 x;
    s32 y;
    s32 textureU;
    u8 brightness;
    char text[16];
} DrawRecord;

static DrawRecord s_records[16];
static s32 s_recordCount;

void DrawSprite(GameOrderingTableEntry *ot, s16 x, s16 y, s16 width, u16 height, u16 textureU,
                u16 textureV, u8 r, u8 g, u8 b, u16 clut, s32 shadeTex,
                s32 semiTrans, u32 flags) {
    (void)ot;
    (void)width;
    (void)height;
    (void)textureV;
    (void)g;
    (void)b;
    (void)clut;
    (void)shadeTex;
    (void)semiTrans;
    (void)flags;
    s_records[s_recordCount++] = (DrawRecord){'S', x, y, textureU, r, ""};
}

void DrawSmallText(s32 x, s16 y, const char *text, u8 r, u8 g, u8 b,
                   u16 clut, s32 flags) {
    DrawRecord *record = &s_records[s_recordCount++];

    (void)g;
    (void)b;
    (void)clut;
    (void)flags;
    *record = (DrawRecord){'T', x, y, 0, r, ""};
    snprintf(record->text, sizeof(record->text), "%s", text);
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
    s_model.maxPower = 765;
    s_model.maxPowerRpm = 7000;
    s_model.maxTorqueWhole = 12;
    s_model.maxTorqueFraction = 3;
    s_model.maxTorqueRpm = 4500;

    g_MenuAltLayout = 1;
    DrawCarEngineSpec(7, 300);
    CHECK(s_recordCount == 0);

    g_MenuAltLayout = 0;
    DrawCarEngineSpec(7, 300);
    CHECK(s_recordCount == 16);
    CHECK(s_records[0].kind == 'S' && s_records[0].x == 0xA1 &&
          s_records[0].y == 0xCC - 7 && s_records[0].brightness == (u8)300);
    CHECK(s_records[2].kind == 'T' && s_records[2].x == 0xD2 &&
          strcmp(s_records[2].text, "765") == 0);
    CHECK(s_records[3].x == 0xE6 && s_records[3].textureU == 0x70);
    CHECK(s_records[5].kind == 'T' && s_records[5].x == 0xF8 &&
          strcmp(s_records[5].text, "7000") == 0);
    CHECK(s_records[9].kind == 'T' && s_records[9].x == 0xD2 &&
          strcmp(s_records[9].text, "12") == 0);
    CHECK(s_records[11].kind == 'T' && s_records[11].x == 0xE1 &&
          strcmp(s_records[11].text, "3") == 0);
    CHECK(s_records[14].kind == 'T' && s_records[14].x == 0x101 &&
          strcmp(s_records[14].text, "4500") == 0);
    CHECK(s_records[15].x == 0x11B && s_records[15].textureU == 0x78);

    puts("car engine spec tests passed");
    return 0;
}
