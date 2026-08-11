#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "game/track.h"
#include "../src/port/input_config.h"

s32 FramesToMilliseconds(s32 frames, s32 millis);
s32 Random15(void);
s32 GetAngleDistance(s32 from, s32 to);
s32 GetAngleDelta(s32 from, s32 to);
s32 InterpolateTrackAngle(s32 pointIndex, s32 weight);
s32 LerpColorChannel(s32 from, s32 to, s32 blend);

u32 g_RandomSeed;
GameTrackPoint *g_TrackPoints;
s32 g_TrackPointCount;

static int failures;

#define EXPECT_EQ(expected, actual) do {                                      \
    s32 expected_value = (s32)(expected);                                     \
    s32 actual_value = (s32)(actual);                                         \
    if (expected_value != actual_value) {                                     \
        fprintf(stderr, "%s:%d: expected %d, got %d\n",                     \
                __FILE__, __LINE__, expected_value, actual_value);             \
        failures++;                                                           \
    }                                                                         \
} while (0)

static void test_time_conversion(void) {
    EXPECT_EQ(0, FramesToMilliseconds(0, 0));
    EXPECT_EQ(960, FramesToMilliseconds(24, 0));
    EXPECT_EQ(1000, FramesToMilliseconds(25, 0));
    EXPECT_EQ(1234, FramesToMilliseconds(30, 34));
    EXPECT_EQ(59999, FramesToMilliseconds(1499, 39));
}

static void test_random15(void) {
    static const s32 expected[] = {
        16838, 5758, 10113, 17515, 31051, 5627, 23010, 7419
    };
    size_t i;

    g_RandomSeed = 1;
    for (i = 0; i < sizeof(expected) / sizeof(expected[0]); i++) {
        EXPECT_EQ(expected[i], Random15());
    }
    EXPECT_EQ((s32)0x9CFBAE39, (s32)g_RandomSeed);

    g_RandomSeed = 0;
    EXPECT_EQ(0, Random15());
    EXPECT_EQ(0x3039, g_RandomSeed);
}

static void test_angle_math(void) {
    EXPECT_EQ(0, GetAngleDistance(0, 0));
    EXPECT_EQ(1, GetAngleDistance(0xFFF, 0));
    EXPECT_EQ(0x800, GetAngleDistance(0, 0x800));
    EXPECT_EQ(0x123, GetAngleDistance(0xF00, 0x023));
    EXPECT_EQ(1, GetAngleDistance(0x1000, -1));

    EXPECT_EQ(1, GetAngleDelta(0xFFF, 0));
    EXPECT_EQ(-1, GetAngleDelta(0, 0xFFF));
    EXPECT_EQ(0x800, GetAngleDelta(0, 0x800));
    EXPECT_EQ(0x7FF, GetAngleDelta(0x801, 0));
    EXPECT_EQ(-1, GetAngleDelta(0x1000, -1));
}

static void test_angle_blending(void) {
    EXPECT_EQ(0x100, BlendAngle(0x100, 0x900, 0));
    EXPECT_EQ(0x900, BlendAngle(0x100, 0x900, 0x400));
    EXPECT_EQ(0, BlendAngle(0xF00, 0x100, 0x200));
    EXPECT_EQ(0, BlendAngle(0x100, 0xF00, 0x200));
    EXPECT_EQ(0x800, BlendAngle(0, 0x800, 0x400));
}

static void test_track_angle_interpolation(void) {
    GameTrackPoint points[3] = {0};

    points[0].angle = 0xF00;
    points[1].angle = 0x100;
    points[2].angle = 0x500;
    g_TrackPoints = points;
    g_TrackPointCount = 3;

    EXPECT_EQ(0, InterpolateTrackAngle(0, 0x200));
    EXPECT_EQ(0x300, InterpolateTrackAngle(1, 0x200));
    EXPECT_EQ(0x200, InterpolateTrackAngle(2, 0x200));
}

static void test_input_config(void) {
    RageInputConfig config;
    char path[] = "/tmp/rage-input-test-XXXXXX";
    const char contents[] = "# custom controls\nUP = I\nCROSS=Space\nUNKNOWN=K\nLEFT=\n";
    int fd;

    RageInputConfigDefaults(&config);
    EXPECT_EQ(12, RageInputButtonIndex("UP"));
    EXPECT_EQ(-1, RageInputButtonIndex("up"));
    EXPECT_EQ(0, strcmp(config.keys[12], "Up"));
    EXPECT_EQ(0, RageInputConfigLoad(&config, "/path/which/does/not/exist"));

    fd = mkstemp(path);
    if (fd < 0 || write(fd, contents, sizeof(contents) - 1) != sizeof(contents) - 1) {
        failures++;
    } else {
        close(fd);
        EXPECT_EQ(2, RageInputConfigLoad(&config, path));
        EXPECT_EQ(0, strcmp(config.keys[12], "I"));
        EXPECT_EQ(0, strcmp(config.keys[6], "Space"));
        EXPECT_EQ(0, strcmp(config.keys[15], "Left"));
        unlink(path);
    }
}

static void test_color_interpolation(void) {
    EXPECT_EQ(10, LerpColorChannel(10, 250, 0));
    EXPECT_EQ(250, LerpColorChannel(10, 250, 0x1000));
    EXPECT_EQ(130, LerpColorChannel(10, 250, 0x800));
    EXPECT_EQ(130, LerpColorChannel(250, 10, 0x800));
}

int main(void) {
    test_time_conversion();
    test_random15();
    test_angle_math();
    test_angle_blending();
    test_track_angle_interpolation();
    test_input_config();
    test_color_interpolation();

    if (failures != 0) {
        fprintf(stderr, "%d characterization assertion(s) failed\n", failures);
        return EXIT_FAILURE;
    }

    puts("game logic characterization tests passed");
    return EXIT_SUCCESS;
}
