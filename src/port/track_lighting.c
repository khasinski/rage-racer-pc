#include "rage/track_lighting.h"

void RageTrackZoneLightColor(int blend, int zoneCode, float out[3]) {
    float amount;
    if (out == 0) return;
    if (blend < 0) blend = 0;
    if (blend > 256) blend = 256;
    amount = (float)blend / 256.0f;
    if (zoneCode != 0) {
        out[0] = out[1] = out[2] = 1.0f - amount * 0.75f;
    } else {
        out[0] = 1.0f;
        out[1] = 1.0f - amount * 0.5f;
        out[2] = 1.0f - amount * 0.75f;
    }
}
