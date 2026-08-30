#include <stdio.h>
#include <stdlib.h>

#include "input_device_select.h"

static int failures;

#define EXPECT_EQ(expected, actual) do {                                       \
    unsigned int got_ = (actual);                                             \
    if ((unsigned int)(expected) != got_) {                                   \
        fprintf(stderr, "%s:%d: expected %u, got %u\n", __FILE__, __LINE__, \
                (unsigned int)(expected), got_);                               \
        failures++;                                                           \
    }                                                                         \
} while (0)

int main(void) {
    RageInputDeviceActivity devices[3] = {
        {11, 0}, {22, 0}, {33, 0},
    };

    EXPECT_EQ(11, SelectActiveInputDevice(devices, 3, 0, 4096));
    EXPECT_EQ(22, SelectActiveInputDevice(devices, 3, 22, 4096));
    devices[2].activity = 30000;
    EXPECT_EQ(33, SelectActiveInputDevice(devices, 3, 22, 4096));
    devices[2].activity = 0;
    devices[0].activity = 4096;
    EXPECT_EQ(33, SelectActiveInputDevice(devices, 3, 33, 4096));
    EXPECT_EQ(11, SelectActiveInputDevice(devices, 2, 33, 4096));
    EXPECT_EQ(0, SelectActiveInputDevice(NULL, 0, 33, 4096));
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
