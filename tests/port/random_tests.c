#include "game/random.h"

#include <stdio.h>
#include <limits.h>

u32 g_RandomSeed;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void) {
    const s32 expected[] = {16838, 5758, 10113, 17515, 31051, 5627, 23010, 7419};
    u32 first = 1;
    u32 second = 123;
    g_RandomSeed = 1;
    for (size_t i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        CHECK(RandomNext(&first) == expected[i]);
        RandomNext(&second);
        CHECK(Random15() == expected[i]);
        CHECK(first == g_RandomSeed);
    }
    CHECK(first == 0x9CFBAE39u);
    u32 restored = first;
    for (int i = 0; i < 100; i++) {
        s32 value = RandomNext(&first);
        RandomNext(&second);
        CHECK(RandomNext(&restored) == value);
        CHECK(restored == first);
    }
    first = 0;
    CHECK(RandomNext(&first) == 0 && first == 0x3039u);
    g_RandomSeed = 1;
    CHECK(RandomIndex(100) == 54);
    CHECK(g_RandomSeed == 0x41C67EA6u);
    first = g_RandomSeed;
    CHECK(RandomIndex(0) == 0 && RandomIndex(-1) == 0);
    CHECK(g_RandomSeed == first);
    g_RandomSeed = 1;
    CHECK(RandomRange(3, 13) == 6);
    CHECK(g_RandomSeed == 0x41C67EA6u);
    g_RandomSeed = 1;
    CHECK(RandomRange(7, 7) == 7);
    CHECK(g_RandomSeed == 0x41C67EA6u);
    first = g_RandomSeed;
    CHECK(RandomRange(9, 4) == 9 && g_RandomSeed == first);
    g_RandomSeed = 1;
    CHECK(RandomRange(INT_MIN, INT_MAX) == INT_MIN + 454);
    return 0;
}
