#include "game/environment.h"
#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)

int main(void) {
    struct {
        u32 skyRowBase, length;
        struct GameEnvironmentCue cues[3];
    } script = {.length = 30, .cues = {{.time = 0, .duration = 4},
        {.time = 15, .duration = 6, .mode = 2}, {.time = -1}}};
    script.cues[0].colors[ENV_FOG].bytes.r = 12;
    script.cues[1].colors[ENV_FOG].bytes.r = 120;
    script.cues[0].spareTarget = 0x8000;
    script.cues[1].spareTarget = 0x8000;
    EnvironmentPalette palettes[ENVIRONMENT_PALETTE_COUNT] = {0};
    palettes[2].colors[0].r = 30;
    Environment a, b;
    CHECK(InitEnvironment(&a, (const void *)&script, sizeof(script), palettes, 0));
    CHECK(InitEnvironment(&b, (const void *)&script, sizeof(script), palettes, 2));
    CHECK(a.clock == 1 && a.mode == 0 && a.frame == 1 && a.duration == 4);
    CHECK(a.colors.fields.slots[ENV_FOG].cur.bytes.r == 93);
    CHECK(a.fogNear == 6000);
    Environment saved = a;
    CHECK(!InitEnvironment(&a, NULL, sizeof(script), palettes, 0));
    CHECK(!InitEnvironment(&a, (const void *)&script, sizeof(script) - 1, palettes, 0));
    CHECK(!InitEnvironment(&a, (const void *)&script, sizeof(script), NULL, 0));
    CHECK(memcmp(&saved, &a, sizeof(a)) == 0);
    SeekEnvironment(&b, -1);
    CHECK(b.clock == 0 && b.mode == 2 && b.fogNear == 32767);
    Environment referenceA = a, referenceB = b;
    for (unsigned tick = 0; tick < 1000; ++tick) {
        TickEnvironment(&a);
        TickEnvironment(&b);
    }
    for (unsigned tick = 0; tick < 1000; ++tick) TickEnvironment(&referenceA);
    for (unsigned tick = 0; tick < 1000; ++tick) TickEnvironment(&referenceB);
    CHECK(memcmp(&a, &referenceA, sizeof(a)) == 0);
    CHECK(memcmp(&b, &referenceB, sizeof(b)) == 0);
    a.enabled = 0;
    saved = a;
    CHECK(!TickEnvironment(&a));
    CHECK(memcmp(&a, &saved, sizeof(a)) == 0);
    CHECK(!TickEnvironment(NULL));
    Environment invalid = b;
    invalid.mode = ENVIRONMENT_PALETTE_COUNT;
    Environment beforeInvalid = invalid;
    CHECK(!TickEnvironment(&invalid));
    CHECK(memcmp(&beforeInvalid, &invalid, sizeof(invalid)) == 0);
    SeekEnvironment(&a, 15);
    CHECK(a.enabled && a.clock == 16 && a.mode == 2 && a.frame == 1);
    CHECK(a.colors.fields.slots[ENV_FOG].cur.bytes.r == 29);
    CHECK(a.clut[0] == 4);
    CHECK(a.fogNear == 32767);
    a.colors.fields.fogEnabled = 0;
    const s32 clock = a.clock;
    CHECK(!TickEnvironment(&a));
    CHECK(a.clock == clock + 1);
    return 0;
}
