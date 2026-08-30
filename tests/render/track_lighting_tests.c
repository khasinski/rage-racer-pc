#include <math.h>
#include <stdio.h>

#include "rage/track_lighting.h"

static int failures;

#define EXPECT_NEAR(expected, actual) do {                                    \
    float expected_value = (expected);                                        \
    float actual_value = (actual);                                            \
    if (fabsf(expected_value - actual_value) > 0.0001f) {                     \
        fprintf(stderr, "%s:%d: expected %.3f, got %.3f\\n", __FILE__,      \
                __LINE__, expected_value, actual_value);                      \
        failures++;                                                           \
    }                                                                         \
} while (0)

static void expect_light(int blend, int code, float red, float green,
                         float blue) {
    float light[3];
    TrackZoneLightColor(blend, code, light);
    EXPECT_NEAR(red, light[0]);
    EXPECT_NEAR(green, light[1]);
    EXPECT_NEAR(blue, light[2]);
}

int main(void) {
    expect_light(0, 1, 1.0f, 1.0f, 1.0f);
    expect_light(256, 1, 0.25f, 0.25f, 0.25f);
    expect_light(256, 0, 1.0f, 0.5f, 0.25f);
    expect_light(-50, 1, 1.0f, 1.0f, 1.0f);
    expect_light(500, 1, 0.25f, 0.25f, 0.25f);
    return failures != 0;
}
