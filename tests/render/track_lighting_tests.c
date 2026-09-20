#include <math.h>
#include <stdio.h>

#include "rage/track_lighting.h"
#include "render/car_lights.h"

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
    EXPECT_NEAR(1, TrackZoneDaylight(-50));
    EXPECT_NEAR(1, TrackZoneDaylight(0));
    EXPECT_NEAR(0.625f, TrackZoneDaylight(128));
    EXPECT_NEAR(0.25f, TrackZoneDaylight(256));
    EXPECT_NEAR(0.25f, TrackZoneDaylight(500));
    /* A warm tunnel keeps its red channel bright. That tint must not stop
     * headlights coming on, even against the brightest daytime sky. */
    {
        CarLights lamps = {0};
        UpdateCarLights(&lamps, 1, TrackZoneDaylight(0), 0, 1);
        EXPECT_NEAR(0, lamps.headlights);
        UpdateCarLights(&lamps, 1, TrackZoneDaylight(256), 0, 1);
        EXPECT_NEAR(1, lamps.headlights);
        EXPECT_NEAR(0.2f, lamps.tail);
        UpdateCarLights(&lamps, 1, TrackZoneDaylight(0), 0, 1);
        EXPECT_NEAR(0, lamps.headlights);
        EXPECT_NEAR(0, lamps.tail);
        UpdateCarLights(&lamps, 1, TrackZoneDaylight(0), 1, 1);
        EXPECT_NEAR(0, lamps.headlights);
        EXPECT_NEAR(1, lamps.stop);
    }
    return failures != 0;
}
