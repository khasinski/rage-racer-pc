#include "game/car.h"
#include "game/car_internal.h"
#include "game/input_internal.h"
#include "game/race.h"
#include "game/state.h"

enum {
    PEDAL_FULLY_PRESSED = 0x100,
    NEGCON_PRESSURE_MAX = 0x6A,
    DIGITAL_ACCELERATOR_MAPPING_SLOT = 2,
    DIGITAL_BRAKE_MAPPING_SLOT = 3,
    NEGCON_ACCELERATOR_MAPPING_SLOT = 10,
    NEGCON_BRAKE_MAPPING_SLOT = 11,
};

static s16 ScaleNegconPedal(s16 pressure) {
    return (s16)((s32)pressure * PEDAL_FULLY_PRESSED / NEGCON_PRESSURE_MAX);
}

static s16 ReadMappedButtonPressure(s32 mappingSlot) {
    return (g_PadHeld & g_PadButtonMapping[mappingSlot]) != 0
               ? PEDAL_FULLY_PRESSED
               : 0;
}

void ReadPlayerCarInput(GameCarDrive *drive) {
    if (g_RacePhase >= 4) {
        drive->acceleratorInput.value = 0;
        drive->brakeInput = 0;
        return;
    }

    if (g_PadType == PAD_TYPE_DIGITAL) {
        drive->acceleratorInput.value =
            ReadMappedButtonPressure(DIGITAL_ACCELERATOR_MAPPING_SLOT);
        drive->brakeInput =
            ReadMappedButtonPressure(DIGITAL_BRAKE_MAPPING_SLOT);
        return;
    }

    if (g_PadType != PAD_TYPE_NEGCON) {
        drive->acceleratorInput.value = 0;
        drive->brakeInput = 0;
        return;
    }

    drive->acceleratorInput.value =
        ReadMappedButtonPressure(NEGCON_ACCELERATOR_MAPPING_SLOT);
    drive->brakeInput = ReadMappedButtonPressure(NEGCON_BRAKE_MAPPING_SLOT);
    switch (g_NegconMappingIndex) {
    case 0:
    case 5:
        drive->acceleratorInput.value = ScaleNegconPedal(g_NegconAnalogI);
        drive->brakeInput = ScaleNegconPedal(g_NegconAnalogII);
        break;
    case 1:
    case 6:
        drive->acceleratorInput.value = ScaleNegconPedal(g_NegconAnalogII);
        drive->brakeInput = ScaleNegconPedal(g_NegconAnalogI);
        break;
    case 2:
        drive->brakeInput = ScaleNegconPedal(g_NegconAnalogL);
        break;
    case 3:
        drive->acceleratorInput.value = ScaleNegconPedal(g_NegconAnalogII);
        drive->brakeInput = ScaleNegconPedal(g_NegconAnalogL);
        break;
    case 4:
    case 7:
        break;
    }
}
