#include "game/car.h"
#include "game/car_internal.h"
#include "game/input_internal.h"
#include "game/race.h"
#include "game/state.h"

static s16 ScaleNegconPedal(s16 input) {
    return (s16)((input << 8) / 106);
}

void ReadPlayerCarInput(GameCarDrive *drive) {
    if (g_RacePhase >= 4) {
        drive->acceleratorInput.value = 0;
        drive->brakeInput = 0;
        return;
    }

    if (g_PadType == PAD_TYPE_DIGITAL) {
        drive->acceleratorInput.value =
            ((g_PadHeld & g_PadButtonMapping[2]) != 0) << 8;
        drive->brakeInput =
            ((g_PadHeld & g_PadButtonMapping[3]) != 0) << 8;
        return;
    }

    if (g_PadType != PAD_TYPE_NEGCON) {
        drive->acceleratorInput.value = 0;
        drive->brakeInput = 0;
        return;
    }

    drive->acceleratorInput.value =
        ((g_PadHeld & g_PadButtonMapping[10]) != 0) << 8;
    drive->brakeInput =
        ((g_PadHeld & g_PadButtonMapping[11]) != 0) << 8;
    switch (g_NegconMappingIndex) {
        case 0:
        case 5:
            drive->acceleratorInput.value =
                ScaleNegconPedal(g_NegconAnalogI);
            drive->brakeInput = ScaleNegconPedal(g_NegconAnalogII);
            break;
        case 1:
        case 6:
            drive->acceleratorInput.value =
                ScaleNegconPedal(g_NegconAnalogII);
            drive->brakeInput = ScaleNegconPedal(g_NegconAnalogI);
            break;
        case 2:
            drive->brakeInput = ScaleNegconPedal(g_NegconAnalogL);
            break;
        case 3:
            drive->acceleratorInput.value =
                ScaleNegconPedal(g_NegconAnalogII);
            drive->brakeInput = ScaleNegconPedal(g_NegconAnalogL);
            break;
        case 4:
        case 7:
            break;
    }
}
