#include "input_device_select.h"

unsigned int SelectActiveInputDevice(
    const RageInputDeviceActivity *devices, size_t count,
    unsigned int currentId, int activationThreshold) {
    unsigned int selected = 0;
    int selectedActivity = activationThreshold;
    int currentConnected = 0;
    size_t index;

    if (devices == NULL || count == 0) return 0;
    for (index = 0; index < count; index++) {
        if (devices[index].id == currentId) currentConnected = 1;
        if (devices[index].activity > selectedActivity) {
            selected = devices[index].id;
            selectedActivity = devices[index].activity;
        } else if (devices[index].id == currentId &&
                   devices[index].activity == selectedActivity &&
                   selectedActivity > activationThreshold) {
            selected = currentId;
        }
    }
    /* No controller moved: retain the current one if it is still connected,
     * otherwise give the first connected controller a stable starting point. */
    if (selected != 0) return selected;
    return currentConnected ? currentId : devices[0].id;
}
