#include "game/environment.h"
#include "game/track.h"
#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)

int main(void) {
    struct {
        u32 skyRowBase, length;
        struct GameEnvironmentCue cues[3];
    } data = {.skyRowBase = SKY_TILE_MAP_ROWS - 2, .length = 100,
              .cues = {{.time = 0}, {.time = 50}, {.time = -1}}};
    const GameEnvironmentScript *script = (const void *)&data;
    CHECK(IsValidEnvironmentScript(script, sizeof(data)));
    const unsigned char *bytes = (const void *)&data;
    unsigned char saved[sizeof(data)];
    memcpy(saved, bytes, sizeof(saved));
    for (size_t size = 0; size < sizeof(data); ++size)
        CHECK(!IsValidEnvironmentScript(script, size));
    CHECK(memcmp(saved, bytes, sizeof(saved)) == 0);
    CHECK(!IsValidEnvironmentScript(NULL, sizeof(data)));
    CHECK(!IsValidEnvironmentScript((const void *)(bytes + 1), sizeof(data) - 1));
    data.skyRowBase = SKY_TILE_MAP_ROWS - 1;
    CHECK(!IsValidEnvironmentScript(script, sizeof(data)));
    data.skyRowBase = 0;
    data.length = 0;
    CHECK(!IsValidEnvironmentScript(script, sizeof(data)));
    data.length = UINT32_MAX;
    CHECK(!IsValidEnvironmentScript(script, sizeof(data)));
    data.length = 100;
    const s32 badTimes[] = {-2, 0, 100, INT32_MAX};
    for (size_t i = 0; i < sizeof(badTimes) / sizeof(badTimes[0]); ++i) {
        data.cues[1].time = badTimes[i];
        CHECK(!IsValidEnvironmentScript(script, sizeof(data)));
    }
    data.cues[1].time = 50;
    data.cues[2].time = 75; /* Missing terminator. */
    CHECK(!IsValidEnvironmentScript(script, sizeof(data)));
    data.cues[2].time = -1;
    for (u16 mode = 0; mode < ENVIRONMENT_PALETTE_COUNT; ++mode) {
        data.cues[1].mode = mode;
        CHECK(IsValidEnvironmentScript(script, sizeof(data)));
    }
    data.cues[1].mode = ENVIRONMENT_PALETTE_COUNT;
    CHECK(!IsValidEnvironmentScript(script, sizeof(data)));
    data.cues[1].mode = 0;
    data.cues[0].time = 1;
    CHECK(!IsValidEnvironmentScript(script, sizeof(data)));
    data.cues[0].time = 0;
    CHECK(IsValidEnvironmentScript(script, sizeof(data)));
    CHECK(EnvironmentTime(-1, 100) == 99);
    CHECK(EnvironmentTime(201, 100) == 1);
    CHECK(EnvironmentTime(INT32_MIN, 100) >= 0);
    CHECK(EnvironmentTime(1, 0) == 0);
    CHECK(EnvironmentCueAt(NULL, 0) == NULL);
    CHECK(EnvironmentCueAt(data.cues, -1) == NULL);
    CHECK(EnvironmentCueAt(data.cues, 0) == &data.cues[0]);
    CHECK(EnvironmentCueAt(data.cues, 49) == &data.cues[0]);
    CHECK(EnvironmentCueAt(data.cues, 50) == &data.cues[1]);
    CHECK(EnvironmentCueAt(data.cues, 99) == &data.cues[1]);
    CHECK(EnvironmentCueFrame(50, 50, 100, 10) == 0);
    CHECK(EnvironmentCueFrame(55, 50, 100, 10) == 5);
    CHECK(EnvironmentCueFrame(99, 50, 100, 10) == 10);
    CHECK(EnvironmentCueFrame(5, 95, 100, 20) == 10);
    CHECK(EnvironmentCueFrame(0, 0, 0, 10) == 0);
    CHECK(EnvironmentCueFrame(0, 0, 100, 0) == 0);
    CHECK(EnvironmentCueFrame(100, 0, 100, 10) == 0);
    EnvironmentPalette from = {0}, to = {0};
    from.colors[0] = (Rgb){0, 31, 16};
    to.colors[0] = (Rgb){31, 0, 16};
    u16 palette[16], paletteBefore[16];
    CHECK(BlendEnvironmentPalette(&from, &to, 2048, palette));
    CHECK(palette[0] == (15 | (15 << 5) | (16 << 10)));
    memcpy(paletteBefore, palette, sizeof(palette));
    CHECK(!BlendEnvironmentPalette(&from, &to, -1, palette));
    CHECK(!BlendEnvironmentPalette(NULL, &to, 0, palette));
    CHECK(!BlendEnvironmentPalette(&from, &to, 0, NULL));
    CHECK(memcmp(paletteBefore, palette, sizeof(palette)) == 0);
    GameEnvironmentColors colors = {0};
    for (s32 slot = 0; slot < ENV_SLOT_COUNT; ++slot) {
        colors.fields.slots[slot].to.bytes.r = 100;
        colors.fields.slots[slot].cur.bytes.unused = 77;
    }
    GameEnvironmentColors second = colors;
    CHECK(BlendEnvironmentColors(&colors, 2, 2048));
    CHECK(colors.fields.slots[ENV_FOG].cur.bytes.r == 50);
    CHECK(colors.fields.slots[ENV_GROUND_NEAR_TOP].cur.bytes.r == 50);
    CHECK(colors.fields.slots[ENV_GROUND_FAR_TOP].cur.bytes.r == 0);
    CHECK(colors.fields.slots[ENV_FOG].cur.bytes.unused == 77);
    CHECK(second.fields.slots[ENV_FOG].cur.bytes.r == 0);
    CHECK(BlendEnvironmentColors(&second, 0, 4096));
    CHECK(second.fields.slots[ENV_GROUND_FAR_TOP].cur.bytes.r == 100);
    CHECK(second.fields.slots[ENV_GROUND_NEAR_TOP].cur.bytes.r == 0);
    GameEnvironmentColors savedColors = colors;
    CHECK(!BlendEnvironmentColors(&colors, 2, 4097));
    CHECK(!BlendEnvironmentColors(NULL, 2, 0));
    CHECK(memcmp(&savedColors, &colors, sizeof(colors)) == 0);
    return 0;
}
