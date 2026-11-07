#include "game/car_control.h"
#include "game/car_shift.h"

void ApplyDriverInput(PlayerCarRuntime *car, const GameCarSpec *spec,
                       const DriverInput *input) {
    /* Automatic shifting uses the previous step's brake value, as in retail. */
    ShiftCarGears(car, spec, input->shiftUp, input->shiftDown);
    UpdateCarSteering(car, &input->steering);
    if (car->verticalMotionState == CAR_VERTICAL_GROUNDED) {
        UpdatePlayerSteeringTarget(car);
    }
    car->drive.acceleratorInput.value = input->throttle;
    car->drive.brakeInput = input->brake;
}
