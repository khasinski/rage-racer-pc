#include "car_lights.h"

#include <math.h>

static int s_enabled = 1;

static float Unit(float value) {
    if (!isfinite(value)) return 1.0f;
    return value < 0.0f ? 0.0f : value > 1.0f ? 1.0f : value;
}

void CarLightsSetEnabled(int enabled) {
    s_enabled = enabled ? 1 : 0;
}

static void ForceLampsOff(CarLights *lights) {
    lights->headlights = 0.0f;
    lights->tail = 0.0f;
    lights->stop = 0.0f;
}

void UpdateCarLights(CarLights *lights, float daylight, float shelter,
                     int braking, float seconds) {
    float brightness, step, target;
    if (!lights) return;
    if (!s_enabled && !isfinite(seconds)) {
        ForceLampsOff(lights);
        return;
    }
    if (s_enabled && (!isfinite(seconds) || seconds <= 0.0f)) return;
    if (!s_enabled && seconds <= 0.0f) {
        ForceLampsOff(lights);
        return;
    }
    brightness = Unit(daylight) * Unit(shelter);
    /* A bridge shadow must not flash the headlights. Require continuous
     * darkness; separate thresholds retain the latch around twilight. */
    if (brightness < 0.28f) {
        lights->darkSeconds += seconds;
        if (lights->darkSeconds >= 0.6f) {
            lights->darkSeconds = 0.6f;
            lights->automatic = 1;
        }
    } else {
        lights->darkSeconds = 0;
        if (brightness > 0.36f) lights->automatic = 0;
    }
    if (!s_enabled) {
        ForceLampsOff(lights);
        return;
    }

    target = lights->automatic ? 1.0f : 0.0f;
    step = seconds * 5.0f;
    lights->headlights = Unit(lights->headlights);
    if (lights->headlights < target) {
        lights->headlights += step;
        if (lights->headlights > target) lights->headlights = target;
    } else {
        lights->headlights -= step;
        if (lights->headlights < target) lights->headlights = target;
    }
    /* Keep the red tail lamps visible in daylight; headlights add a little
     * more intensity at night. */
    lights->tail = 0.20f + lights->headlights * 0.20f;
    /* Brake lamps switch on immediately and linger for a tenth of a second
     * after release. That short bulb-like decay avoids strobing when the rival
     * speed controller alternates its brake request between adjacent ticks. */
    lights->stop = Unit(lights->stop);
    lights->stop = braking ? 1.0f : fmaxf(0.0f, lights->stop - seconds * 10.0f);
}
