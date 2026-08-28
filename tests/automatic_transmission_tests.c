#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/asset.h"
#include "game/car.h"
#include "rage/automatic_transmission.h"

static int failures;
#define EXPECT(condition) do { if (!(condition)) {                              \
    fprintf(stderr, "%s:%d: expectation failed: %s\n", __FILE__, __LINE__,  \
            #condition); failures++;                                            \
} } while (0)

int main(void) {
    CarModelAsset automaticModel = {0};
    CarModelAsset manualOnlyModel = {0};
    GameCarSpec source = {0};
    GameCarSpec untouched;
    GameCarSpec *prepared;
    int gear;

    automaticModel.transmissionAvailable = 1;
    source.revLimit = 9000;
    source.topGear = 6;
    source.automaticAccelerationScale = 6;
    source.gearRatio[1] = 120;
    source.gearRatio[2] = 155;
    source.gearRatio[3] = 195;
    source.gearRatio[4] = 240;
    source.gearRatio[5] = 295;
    source.gearRatio[6] = 360;
    memcpy(&untouched, &source, sizeof(source));

    EXPECT(RageAutomaticTransmissionSelectable(&manualOnlyModel));
    EXPECT(RageAutomaticTransmissionSpec(&source, 0, &manualOnlyModel) ==
           &source);
    EXPECT(RageAutomaticTransmissionSpec(&source, 1, &automaticModel) ==
           &source);

    prepared = RageAutomaticTransmissionSpec(&source, 1, &manualOnlyModel);
    EXPECT(prepared != &source);
    EXPECT(prepared->automaticAccelerationScale == 990);
    EXPECT(memcmp(&source, &untouched, sizeof(source)) == 0);
    for (gear = 1; gear < prepared->topGear; gear++) {
        EXPECT(prepared->shiftPoints[gear - 1].upshiftSpeed > 0);
        if (gear > 1)
            EXPECT(prepared->shiftPoints[gear - 1].upshiftSpeed >
                   prepared->shiftPoints[gear - 2].upshiftSpeed);
    }
    for (gear = 2; gear <= prepared->topGear; gear++) {
        EXPECT(prepared->shiftPoints[gear - 1].downshiftSpeed > 0);
        EXPECT(prepared->shiftPoints[gear - 1].downshiftSpeed <
               prepared->shiftPoints[gear - 2].upshiftSpeed);
    }
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
