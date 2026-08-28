#include "rage/automatic_transmission.h"

#include <stdint.h>
#include <string.h>

#include "game/asset.h"
#include "game/car.h"

static GameCarSpec s_automaticSpec;

int RageAutomaticTransmissionSelectable(const CarModelAsset *asset) {
    return asset != NULL;
}

static int ShiftTableIsUsable(const GameCarSpec *spec) {
    int gear;
    int previousUp = 0;
    int previousDown = 0;
    if (spec->topGear < 2 || spec->topGear > 6) return 0;
    for (gear = 1; gear < spec->topGear; gear++) {
        int up = spec->shiftPoints[gear - 1].upshiftSpeed;
        if (up <= previousUp) return 0;
        previousUp = up;
    }
    for (gear = 2; gear <= spec->topGear; gear++) {
        int down = spec->shiftPoints[gear - 1].downshiftSpeed;
        if (down <= previousDown ||
            down >= spec->shiftPoints[gear - 2].upshiftSpeed) return 0;
        previousDown = down;
    }
    return 1;
}

static int SpeedAtRpm(const GameCarSpec *spec, int gear, int rpm) {
    int64_t numerator;
    if (gear < 1 || gear > 6 || spec->gearRatio[gear] <= 0) return 1;
    numerator = (int64_t)rpm * spec->gearRatio[gear] * 1168;
    return (int)(numerator / (160 * 10000));
}

static void BuildShiftTable(GameCarSpec *spec) {
    int gear;
    for (gear = 1; gear < spec->topGear; gear++) {
        int percent = 72 + gear * 4;
        spec->shiftPoints[gear - 1].upshiftSpeed =
            (s16)SpeedAtRpm(spec, gear, spec->revLimit * percent / 100);
    }
    for (gear = 2; gear <= spec->topGear; gear++) {
        int percent = 50 + gear * 4;
        int down = SpeedAtRpm(spec, gear, spec->revLimit * percent / 100);
        int ceiling = spec->shiftPoints[gear - 2].upshiftSpeed - 1;
        if (down > ceiling) down = ceiling;
        if (down <= spec->shiftPoints[gear - 2].downshiftSpeed)
            down = spec->shiftPoints[gear - 2].downshiftSpeed + 1;
        spec->shiftPoints[gear - 1].downshiftSpeed = (s16)down;
    }
}

GameCarSpec *RageAutomaticTransmissionSpec(GameCarSpec *source,
                                            int automaticSelected,
                                            const CarModelAsset *model) {
    if (source == NULL || model == NULL || !automaticSelected ||
        model->transmissionAvailable != 0) return source;
    memcpy(&s_automaticSpec, source, sizeof(s_automaticSpec));
    if (s_automaticSpec.automaticAccelerationScale < 800 ||
        s_automaticSpec.automaticAccelerationScale > 1200)
        s_automaticSpec.automaticAccelerationScale = 990;
    if (!ShiftTableIsUsable(&s_automaticSpec)) BuildShiftTable(&s_automaticSpec);
    return &s_automaticSpec;
}
