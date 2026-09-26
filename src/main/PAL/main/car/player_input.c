#include "game/car.h"
#include "game/car_control.h"
#include "game/car_internal.h"
#include "game/input_internal.h"
#include "game/integer.h"
#include "game/race.h"
#include "game/state.h"

enum {
    PEDAL_FULLY_PRESSED = 0x100,
    NEGCON_PRESSURE_MAX = 0x6A,
    DIGITAL_ACCELERATOR_MAPPING_SLOT = 2,
    DIGITAL_BRAKE_MAPPING_SLOT = 3,
    NEGCON_ACCELERATOR_MAPPING_SLOT = 10,
    NEGCON_BRAKE_MAPPING_SLOT = 11,
    SHIFT_MAPPING_STRIDE = 8,
    SHIFT_UP_MAPPING_SLOT = 4,
    SHIFT_DOWN_MAPPING_SLOT = 5,
    NEGCON_STEERING_SCALE = 13 * 512,
};

static SteeringInput ReadSteeringInput(void) {
    SteeringInput input = {0};
    if (g_PadType == PAD_TYPE_DIGITAL) {
        input.mode = STEERING_DIGITAL;
        input.left = (g_PadHeld & g_PadButtonMapping[0]) != 0;
        input.right = (g_PadHeld & g_PadButtonMapping[1]) != 0;
    } else if (g_PadType == PAD_TYPE_NEGCON) {
        input.mode = STEERING_ANALOG;
        input.angle = (g_NegconSteer * NEGCON_STEERING_SCALE) / GetNegconSteerRange();
    }
    return input;
}

static SteeringInput ReadPlayerSteering(void) {
    if (g_RacePhase < RACE_PHASE_ACTIVE) return (SteeringInput){0};
    if (g_RacePhase >= RACE_PHASE_FINISHED || g_PlayerAutoSteer)
        return (SteeringInput){.mode = STEERING_AUTOMATIC};
    return ReadSteeringInput();
}

void UpdateCarBodyRoll(PlayerCarRuntime *car) {
    const SteeringInput input = ReadPlayerSteering();
    UpdateCarSteering(car, &input);
}

void ShiftPlayerGears(PlayerCarRuntime *car, int useAlternateMapping) {
    s32 base = useAlternateMapping ? SHIFT_MAPPING_STRIDE : 0;
    ShiftCarGears(car, g_CarSpec,
        (g_PadPressed & g_PadButtonMapping[SHIFT_UP_MAPPING_SLOT + base]) != 0,
        (g_PadPressed & g_PadButtonMapping[SHIFT_DOWN_MAPPING_SLOT + base]) != 0);
}

static s16 ScaleNegconPedal(s16 pressure, int bounded) {
    if (bounded) {
        if (pressure < 0) pressure = 0;
        if (pressure > NEGCON_PRESSURE_MAX) pressure = NEGCON_PRESSURE_MAX;
    }
    return WrapSigned16(
        (int64_t)pressure * PEDAL_FULLY_PRESSED / NEGCON_PRESSURE_MAX);
}

static s16 ReadMappedButtonPressure(s32 mappingSlot) {
    return (g_PadHeld & g_PadButtonMapping[mappingSlot]) != 0
               ? PEDAL_FULLY_PRESSED
               : 0;
}

static void ReadPedals(DriverInput *input, int bounded) {
    if (g_PadType == PAD_TYPE_DIGITAL) {
        input->throttle =
            ReadMappedButtonPressure(DIGITAL_ACCELERATOR_MAPPING_SLOT);
        input->brake =
            ReadMappedButtonPressure(DIGITAL_BRAKE_MAPPING_SLOT);
        return;
    }

    if (g_PadType != PAD_TYPE_NEGCON) {
        input->throttle = 0;
        input->brake = 0;
        return;
    }

    input->throttle =
        ReadMappedButtonPressure(NEGCON_ACCELERATOR_MAPPING_SLOT);
    input->brake = ReadMappedButtonPressure(NEGCON_BRAKE_MAPPING_SLOT);
    switch (g_NegconMappingIndex) {
    case 0:
    case 5:
        input->throttle = ScaleNegconPedal(g_NegconAnalogI, bounded);
        input->brake = ScaleNegconPedal(g_NegconAnalogII, bounded);
        break;
    case 1:
    case 6:
        input->throttle = ScaleNegconPedal(g_NegconAnalogII, bounded);
        input->brake = ScaleNegconPedal(g_NegconAnalogI, bounded);
        break;
    case 2:
        input->brake = ScaleNegconPedal(g_NegconAnalogL, bounded);
        break;
    case 3:
        input->throttle = ScaleNegconPedal(g_NegconAnalogII, bounded);
        input->brake = ScaleNegconPedal(g_NegconAnalogL, bounded);
        break;
    case 4:
    case 7:
    default:
        /* Invalid saved mappings retain the button-based fallback sampled
         * above, just like layouts without analog pedal assignments. */
        break;
    }
}

void ReadPlayerCarInput(GameCarDrive *drive) {
    DriverInput input = {0};
    if (g_RacePhase < RACE_PHASE_FINISHED) ReadPedals(&input, 0);
    drive->acceleratorInput.value = input.throttle;
    drive->brakeInput = input.brake;
}

static DriverInput ReadControls(SteeringInput steering, int bounded) {
    DriverInput input = {0};
    ReadPedals(&input, bounded);
    const s32 base = g_PadType == PAD_TYPE_NEGCON ? SHIFT_MAPPING_STRIDE : 0;
    input.steering = steering;
    input.shiftUp = (g_PadPressed & g_PadButtonMapping[SHIFT_UP_MAPPING_SLOT + base]) != 0;
    input.shiftDown = (g_PadPressed & g_PadButtonMapping[SHIFT_DOWN_MAPPING_SLOT + base]) != 0;
    return input;
}

DriverInput ReadDriverInput(void) {
    DriverInput input = ReadControls(ReadPlayerSteering(), 0);
    if (g_RacePhase >= RACE_PHASE_FINISHED) input.throttle = input.brake = 0;
    return input;
}

DriverInput ReadCarControls(void) {
    DriverInput input = ReadControls(ReadSteeringInput(), 1);
    if (input.throttle < 0) input.throttle = 0;
    if (input.throttle > PEDAL_FULLY_PRESSED) input.throttle = PEDAL_FULLY_PRESSED;
    if (input.brake < 0) input.brake = 0;
    if (input.brake > PEDAL_FULLY_PRESSED) input.brake = PEDAL_FULLY_PRESSED;
    if (input.steering.angle < -NEGCON_STEERING_SCALE) input.steering.angle = -NEGCON_STEERING_SCALE;
    if (input.steering.angle > NEGCON_STEERING_SCALE) input.steering.angle = NEGCON_STEERING_SCALE;
    return input;
}
