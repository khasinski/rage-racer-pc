#include "game/track_look.h"
#include <stdio.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void) {
    TrackLook source = {.environmentStart = 12345};
    for (s32 model = 0; model < CAR_MODEL_BANK_ENTRY_COUNT; ++model)
        source.models[model] = (CarModelRenderParams){model, (u16)(model + 20), (u16)0xfffe, (s16)(model + 30)};
    u8 bytes[sizeof(source) + 1];
    memcpy(bytes + 1, &source, sizeof(source));
    TrackLook look = {0};
    CHECK(ReadTrackLook(bytes + 1, sizeof(source), &look));
    CHECK(memcmp(&look, &source, sizeof(look)) == 0);
    for (size_t size = 0; size < sizeof(source); ++size) {
        CHECK(!ReadTrackLook(bytes + 1, size, &look));
        CHECK(memcmp(&look, &source, sizeof(look)) == 0);
    }
    CHECK(!ReadTrackLook(NULL, sizeof(source), &look));
    CHECK(!ReadTrackLook(bytes, sizeof(bytes), NULL));
    memset(bytes, 0, sizeof(bytes));
    CHECK(look.environmentStart == 12345);
    for (s32 course = 0; course < 4; ++course) {
        for (s32 slot = 0; slot < 11; ++slot) {
            const s32 model = g_CarModelByCourse[course][slot];
            RivalLook rival = {0};
            CHECK(ReadRivalLook(&look, course, slot, 35, &rival));
            CHECK(rival.shape.offsetX == model && rival.shape.offsetY == model + 20);
            CHECK(rival.shape.offsetZ == -2 && rival.shape.horizon == model + 30);
            CHECK(rival.bodyMesh == (u32)g_CarModelBankTable[model][0]);
            CHECK(rival.palette == g_CarModelBankTable[model][1]);
            const RivalLook saved = rival;
            CHECK(!ReadRivalLook(&look, course, slot, (s32)rival.bodyMesh + 4, &rival));
            CHECK(memcmp(&rival, &saved, sizeof(rival)) == 0);
        }
    }
    RivalLook rival = {0}, saved = rival;
    CHECK(!ReadRivalLook(&look, -1, 0, 35, &rival));
    CHECK(!ReadRivalLook(&look, 4, 0, 35, &rival));
    CHECK(!ReadRivalLook(&look, 0, -1, 35, &rival));
    CHECK(!ReadRivalLook(&look, 0, 11, 35, &rival));
    CHECK(!ReadRivalLook(&look, 0, 0, -1, &rival));
    CHECK(!ReadRivalLook(NULL, 0, 0, 35, &rival));
    CHECK(memcmp(&rival, &saved, sizeof(rival)) == 0);
    return 0;
}
