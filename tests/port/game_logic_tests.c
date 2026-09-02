#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "common.h"
#include "game/track.h"
#include "input_config.h"
#include "port_config.h"
#include "runtime_config.h"
#include "platform_paths.h"

/* Random15 stamps its trace with where the game had got to. The pure logic
 * library carries no scene state of its own, so the test supplies it. */
int g_FrameCounter;
int g_SceneId;
int g_SceneTimer;

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
    EXPECT_EQ(-0x800, GetAngleDelta(0x800, 0));
    EXPECT_EQ(0x7FF, GetAngleDelta(0x801, 0));
    EXPECT_EQ(-1, GetAngleDelta(0x1000, -1));
    EXPECT_EQ(0x123, GetAngleDelta(0xF00, 0x023));
    EXPECT_EQ(-0x123, GetAngleDelta(0x023, 0xF00));
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

    InputConfigDefaults(&config);
    EXPECT_EQ(12, InputButtonIndex("UP"));
    EXPECT_EQ(-1, InputButtonIndex("up"));
    EXPECT_EQ(0, strcmp(config.keys[12], "Up"));
    EXPECT_EQ(0, InputConfigLoad(&config, "/path/which/does/not/exist"));

    fd = mkstemp(path);
    if (fd < 0 || write(fd, contents, sizeof(contents) - 1) != sizeof(contents) - 1) {
        failures++;
    } else {
        close(fd);
        EXPECT_EQ(2, InputConfigLoad(&config, path));
        EXPECT_EQ(0, strcmp(config.keys[12], "I"));
        EXPECT_EQ(0, strcmp(config.keys[6], "Space"));
        EXPECT_EQ(0, strcmp(config.keys[15], "Left"));
        unlink(path);
    }
}

static void test_port_config(void) {
    RagePortConfig config;
    char path[] = "/tmp/rage-port-test-XXXXXX";
    const char contents[] =
        "# video settings\n"
        "[video]\n"
        "renderer = modern\n"
        "internal_scale = 3.5\n"
        "aspect = 16:9\n"
        "fps = 120\n"
        "texture_filter = linear\n"
        "post = fxaa\n"
        "bloom = on\n"
        "grading = vibrant\n"
        "draw_distance = nonsense\n"
        "unknown.key = 1\n";
    int fd;

    PortConfigDefaults(&config);
    EXPECT_EQ(RAGE_RENDERER_CLASSIC, config.renderer);
    EXPECT_EQ(RAGE_MODERN_FPS_LOGIC, config.modernFps);
    fd = mkstemp(path);
    if (fd < 0 || write(fd, contents, sizeof(contents) - 1) != sizeof(contents) - 1) {
        failures++;
    } else {
        char *arguments[] = {"rage-test", "--config", path};
        close(fd);
        EXPECT_EQ(1, RuntimeConfigInit(3, arguments));
        EXPECT_EQ(8, PortConfigApplyRuntime(&config));
        EXPECT_EQ(RAGE_RENDERER_MODERN, config.renderer);
        EXPECT_EQ(RAGE_MODERN_ASPECT_16_9, config.modernAspect);
        EXPECT_EQ(120, config.modernFps);
        EXPECT_EQ(1, config.modernTextureFilterLinear);
        EXPECT_EQ(RAGE_MODERN_POST_FXAA, config.modernPost);
        EXPECT_EQ(1, config.modernGrading);
        EXPECT_EQ(35, (s32)(config.modernInternalScale * 10.0f));
        /* invalid value keeps the default */
        EXPECT_EQ(10, (s32)(config.modernDrawDistance * 10.0f));
        unlink(path);
    }
    {
        RagePortConfig zeroDistance;
        /* Without an explicit --config, Init falls back to whatever
         * rage-port.ini it can find near the binary, and the one shipped in
         * the repository sets every video key. Point these cases at an empty
         * file so they measure their own --set rather than the environment. */
        char emptyPath[] = "/tmp/rage-empty-XXXXXX";
        int emptyFd = mkstemp(emptyPath);
        char *zeroArguments[] = {
            "rage-test", "--config", emptyPath, "--set", "video.draw_distance=0"};
        char *booleanArguments[] = {
            "rage-test", "--set", "feature.enabled=False"};
        char *invalidSet[] = {"rage-test", "--set", "broken"};
        char *missingConfig[] = {
            "rage-test", "--config", "/path/which/does/not/exist"};
        PortConfigDefaults(&zeroDistance);
        if (emptyFd >= 0) close(emptyFd);
        EXPECT_EQ(1, RuntimeConfigInit(5, zeroArguments));
        EXPECT_EQ(1, PortConfigApplyRuntime(&zeroDistance));
        EXPECT_EQ(0, (s32)(zeroDistance.modernDrawDistance * 10.0f));
        EXPECT_EQ(1, RuntimeConfigInit(3, booleanArguments));
        EXPECT_EQ(0, RuntimeConfigEnabled("feature.enabled"));
        EXPECT_EQ(0, RuntimeConfigInit(3, invalidSet));
        EXPECT_EQ(0, RuntimeConfigInit(3, missingConfig));
        unlink(emptyPath);
    }
}

static void test_platform_config_path(void) {
    char root[] = "/tmp/rage-path-test-XXXXXX";
    char directory[256], filePath[320], found[320];
#ifdef __APPLE__
    const char *previous = getenv("HOME");
#else
    const char *previous = getenv("XDG_CONFIG_HOME");
#endif
    char *saved = previous != NULL ? strdup(previous) : NULL;
    FILE *file;
    if (mkdtemp(root) == NULL) {
        failures++;
        return;
    }
#ifdef __APPLE__
    snprintf(directory, sizeof(directory), "%s/Library", root);
    mkdir(directory, 0700);
    snprintf(directory, sizeof(directory), "%s/Library/Application Support",
             root);
    mkdir(directory, 0700);
    snprintf(directory, sizeof(directory),
             "%s/Library/Application Support/Rage Racer", root);
    setenv("HOME", root, 1);
#else
    snprintf(directory, sizeof(directory), "%s/rage-racer", root);
    setenv("XDG_CONFIG_HOME", root, 1);
#endif
    snprintf(filePath, sizeof(filePath), "%s/rage-port.ini", directory);
    if (mkdir(directory, 0700) != 0 || (file = fopen(filePath, "wb")) == NULL) {
        failures++;
    } else {
        fclose(file);
        EXPECT_EQ(1, PlatformFindConfigFile(NULL, "rage-port.ini", found,
                                                sizeof(found)));
        EXPECT_EQ(0, strcmp(filePath, found));
    }
    if (saved != NULL) {
#ifdef __APPLE__
        setenv("HOME", saved, 1);
#else
        setenv("XDG_CONFIG_HOME", saved, 1);
#endif
        free(saved);
    } else {
#ifdef __APPLE__
        unsetenv("HOME");
#else
        unsetenv("XDG_CONFIG_HOME");
#endif
    }
    unlink(filePath);
    rmdir(directory);
#ifdef __APPLE__
    snprintf(directory, sizeof(directory), "%s/Library/Application Support",
             root);
    rmdir(directory);
    snprintf(directory, sizeof(directory), "%s/Library", root);
    rmdir(directory);
#endif
    rmdir(root);
}

static void test_portable_state_path(void) {
    char root[] = "/tmp/rage-portable-state-test-XXXXXX";
    char executable[320], bundleExecutable[320], card[326], found[320];

    if (mkdtemp(root) == NULL) {
        failures++;
        return;
    }
    snprintf(executable, sizeof(executable), "%s/game", root);
    mkdir(executable, 0700);
    EXPECT_EQ(0, PlatformExistingPortableStateDirectory(
                     executable, found, sizeof(found)));
    snprintf(card, sizeof(card), "%s/bu00", executable);
    mkdir(card, 0700);
    EXPECT_EQ(1, PlatformExistingPortableStateDirectory(
                     executable, found, sizeof(found)));
    EXPECT_EQ(0, strcmp(executable, found));
    rmdir(card);

    snprintf(bundleExecutable, sizeof(bundleExecutable),
             "%s/Rage Racer.app/Contents/MacOS", root);
    snprintf(card, sizeof(card), "%s/bu00", root);
    mkdir(card, 0700);
    EXPECT_EQ(1, PlatformExistingPortableStateDirectory(
                     bundleExecutable, found, sizeof(found)));
    EXPECT_EQ(0, strcmp(root, found));
    rmdir(card);
    rmdir(executable);
    rmdir(root);
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
    test_port_config();
    test_platform_config_path();
    test_portable_state_path();
    test_color_interpolation();

    if (failures != 0) {
        fprintf(stderr, "%d characterization assertion(s) failed\n", failures);
        return EXIT_FAILURE;
    }

    puts("game logic characterization tests passed");
    return EXIT_SUCCESS;
}
