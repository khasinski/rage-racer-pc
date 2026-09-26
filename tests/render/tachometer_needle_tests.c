#include <stdio.h>
#include <string.h>

#include "game/car.h"
#include "game/render.h"
#include "game/render_internal.h"

static GameCarSpec s_CarSpec;
GameFrameContext g_FrameContexts[2];
GameSpriteDesc g_TachoNeedleSprite;

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    CarTachometerSpec *spec = &s_CarSpec.tachometer;
    RaceHudPackets *first = &g_FrameContexts[0].layout.raceHud;
    RaceHudPackets *second = &g_FrameContexts[1].layout.raceHud;

    memset(&s_CarSpec, 0, sizeof(s_CarSpec));
    memset(g_FrameContexts, 0, sizeof(g_FrameContexts));
    memset(&g_TachoNeedleSprite, 0, sizeof(g_TachoNeedleSprite));
    spec->needleX = 100;
    spec->needleY = 200;
    spec->faceDX = 7;
    spec->faceDY = 9;
    g_TachoNeedleSprite.w = 40;
    g_TachoNeedleSprite.h = 24;
    g_TachoNeedleSprite.u0 = 11;
    g_TachoNeedleSprite.v0 = 13;
    g_TachoNeedleSprite.clut = 0x456;
    g_TachoNeedleSprite.semiTrans = 1;

    const GameSpriteDesc unchanged = g_TachoNeedleSprite;
    BuildTachometerFace(spec);

    CHECK(memcmp(&unchanged, &g_TachoNeedleSprite, sizeof(unchanged)) == 0);
    CHECK(first->tachometerFace.x0 == 107 &&
          first->tachometerFace.y0 == 209);
    CHECK(first->tachometerFace.w == 40 && first->tachometerFace.h == 24);
    CHECK(first->tachometerFace.u0 == 11 &&
          first->tachometerFace.v0 == 13 &&
          first->tachometerFace.clut == 0x456);
    CHECK(memcmp(&first->tachometerFace, &second->tachometerFace,
                 sizeof(first->tachometerFace)) == 0);
    CHECK((first->tachometerFace.code & 1) == 0);

    CHECK(memcmp(first->tachometerDrawModes, second->tachometerDrawModes,
                 sizeof(first->tachometerDrawModes)) == 0);
    CHECK(memcmp(&first->tachometerDrawModes[0],
                 &first->tachometerDrawModes[1],
                 sizeof(first->tachometerDrawModes[0])) != 0);

CarTachometerSpec other = *spec;
other.needleX = 10; other.needleY = 20;
BuildTachometerFace(&other);
CHECK(first->tachometerFace.x0 == 17 && first->tachometerFace.y0 == 29);
CHECK(memcmp(&unchanged, &g_TachoNeedleSprite, sizeof(unchanged)) == 0);
BuildTachometerFace(spec);
CHECK(first->tachometerFace.x0 == 107 && first->tachometerFace.y0 == 209);
    puts("tachometer face and frame packets passed");
    return 0;
}
