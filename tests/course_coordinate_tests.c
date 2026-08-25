#include <stdio.h>
#include <stdlib.h>

#include "course_coordinate.h"

static int failures;

#define EXPECT_EQ(expected, actual) do {                                      \
    int expected_value = (expected);                                          \
    int actual_value = (actual);                                              \
    if (expected_value != actual_value) {                                     \
        fprintf(stderr, "%s:%d: expected %d, got %d\\n", __FILE__,        \
                __LINE__, expected_value, actual_value);                      \
        failures++;                                                           \
    }                                                                         \
} while (0)

int main(void) {
    /* -64725 is the wrapped height used by the Lakeside Gate waterfall. */
    EXPECT_EQ(811, RageCourseCoordinateNearReference(-64725, 600));
    EXPECT_EQ(65546, RageCourseCoordinateNearReference(10, 65500));
    EXPECT_EQ(-36, RageCourseCoordinateNearReference(65500, 10));
    if (failures != 0) return EXIT_FAILURE;
    puts("course coordinate tests passed");
    return EXIT_SUCCESS;
}
