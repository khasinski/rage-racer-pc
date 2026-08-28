#ifndef RAGE_INPUT_DEVICE_SELECT_H
#define RAGE_INPUT_DEVICE_SELECT_H

#include <stddef.h>

typedef struct RageInputDeviceActivity {
    unsigned int id;
    int activity;
} RageInputDeviceActivity;

unsigned int RageSelectActiveInputDevice(
    const RageInputDeviceActivity *devices, size_t count,
    unsigned int currentId, int activationThreshold);

#endif
