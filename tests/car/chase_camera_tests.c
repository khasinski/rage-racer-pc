#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rage/chase_camera.h"

static int failures;

#define EXPECT_EQ(expected, actual) do { if ((expected) != (actual)) {      \
    fprintf(stderr, "%s:%d: expected %d, got %d\n", __FILE__, __LINE__, \
            (expected), (actual)); failures++;                              \
} } while (0)

const char *RuntimeConfigGet(const char *key) {
    const char *override = getenv("CHASE_TEST_VALUE");
    if (!strcmp(key, "camera.chase_turn_lookahead"))
        return override != NULL ? override : "0.5";
    return NULL;
}

int main(void) {
    if (getenv("CHASE_TEST_VALUE") != NULL) {
        EXPECT_EQ(0, ChaseCameraYawOffset(4096));
        return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    EXPECT_EQ(0, ChaseCameraYawOffset(0));
    EXPECT_EQ(170, ChaseCameraYawOffset(4096));
    EXPECT_EQ(-170, ChaseCameraYawOffset(-4096));
    EXPECT_EQ(170, ChaseCameraYawOffset(8192));
    EXPECT_EQ(-170, ChaseCameraYawOffset(-8192));
    EXPECT_EQ(85, ChaseCameraYawOffset(2048));
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
