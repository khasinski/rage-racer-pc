#include "game/car_select_controller.h"
#include "game/pad.h"

#define EXPECT_EQ(expected, actual) do { if ((expected) != (actual)) return __LINE__; } while (0)

int main(void) {
    CarSelectInputResult car;
    CarSelectScreenState carScreen;
    CarSelectScreenInput carInput;
    CarSelectScreenResult carResult;
    CustomizeInputResult customize;
    CustomizeScreenState screen;
    CustomizeScreenInput screenInput;
    CustomizeScreenResult screenResult;

    car = CarSelectHandleInput(0, 0, -1, 0, 0, 0, PAD_UP);
    EXPECT_EQ(2, car.selection);
    car = CarSelectHandleInput(2, 0, -1, 0, 0, 0, PAD_CONFIRM);
    EXPECT_EQ(CAR_SELECT_COMMAND_BACK, car.command);
    car = CarSelectHandleInput(2, 1, -1, 0, 0, 0, PAD_CONFIRM);
    EXPECT_EQ(CAR_SELECT_COMMAND_CAR_SHOP_UNAVAILABLE, car.command);
    car = CarSelectHandleInput(2, 1, 7, 0, 0, 0, PAD_CONFIRM);
    EXPECT_EQ(CAR_SELECT_COMMAND_CAR_SHOP, car.command);
    car = CarSelectHandleInput(3, 1, 7, 1, 2, 3, PAD_CONFIRM);
    EXPECT_EQ(CAR_SELECT_COMMAND_ENGINEER_SHOP_UNAVAILABLE, car.command);
    car = CarSelectHandleInput(3, 1, 7, 1, 3, 3, PAD_CONFIRM);
    EXPECT_EQ(CAR_SELECT_COMMAND_ENGINEER_SHOP, car.command);

    carScreen = (CarSelectScreenState){CAR_SELECT_ACTIVE, 2};
    carInput = (CarSelectScreenInput){PAD_CONFIRM, 1, -1, 0, 0, 0};
    carResult = CarSelectReduceInput(&carScreen, &carInput);
    EXPECT_EQ(CAR_SELECT_COMMAND_CAR_SHOP_UNAVAILABLE, carResult.command);
    carScreen = (CarSelectScreenState){CAR_SELECT_SHOP_UNAVAILABLE, 2};
    carInput.pressed = PAD_CANCEL;
    carResult = CarSelectReduceInput(&carScreen, &carInput);
    EXPECT_EQ(CAR_SELECT_ACTIVE, carResult.state.phase);

    customize = CustomizeHandleInput(2, 0, 1, PAD_CONFIRM);
    EXPECT_EQ(CUSTOMIZE_COMMAND_BACK, customize.command);
    customize = CustomizeHandleInput(2, 1, 1, PAD_CONFIRM);
    EXPECT_EQ(CUSTOMIZE_COMMAND_DESIGN, customize.command);
    customize = CustomizeHandleInput(1, 1, 0, PAD_CONFIRM);
    EXPECT_EQ(CUSTOMIZE_COMMAND_TRANSMISSION_UNAVAILABLE, customize.command);

    screen = (CustomizeScreenState){CUSTOMIZE_ACTIVE, 0, 0, 0};
    screenInput = (CustomizeScreenInput){PAD_DOWN | PAD_CONFIRM, 1, 1};
    screenResult = CustomizeReduceInput(&screen, &screenInput);
    EXPECT_EQ(1, screenResult.state.selection);
    EXPECT_EQ(CUSTOMIZE_COMMAND_TRANSMISSION, screenResult.command);

    screen = (CustomizeScreenState){CUSTOMIZE_TIRE_PROMPT, 0, 2, 0};
    screenInput.pressed = PAD_RIGHT | PAD_CONFIRM;
    screenResult = CustomizeReduceInput(&screen, &screenInput);
    EXPECT_EQ(CUSTOMIZE_CONFIRM_TIRES, screenResult.state.phase);
    EXPECT_EQ(1, screenResult.state.modalCursor);
    EXPECT_EQ(0x23, screenResult.state.confirmTimer);
    EXPECT_EQ(CUSTOMIZE_EFFECT_ACCEPT, screenResult.effects);

    screen = (CustomizeScreenState){CUSTOMIZE_TRANSMISSION_PROMPT, 0, 0, 0};
    screenInput.pressed = PAD_RIGHT | PAD_CONFIRM | PAD_CANCEL;
    screenResult = CustomizeReduceInput(&screen, &screenInput);
    EXPECT_EQ(CUSTOMIZE_ACTIVE, screenResult.state.phase);
    EXPECT_EQ(1, screenResult.state.modalCursor);
    EXPECT_EQ(CUSTOMIZE_EFFECT_ACCEPT | CUSTOMIZE_EFFECT_CANCEL |
                  CUSTOMIZE_EFFECT_APPLY_TRANSMISSION,
              screenResult.effects);

    screen = (CustomizeScreenState){CUSTOMIZE_TRANSMISSION_UNAVAILABLE, 0, 0, 0};
    screenInput.pressed = PAD_CANCEL;
    screenResult = CustomizeReduceInput(&screen, &screenInput);
    EXPECT_EQ(CUSTOMIZE_CLOSE_UNAVAILABLE, screenResult.state.phase);

    screen = (CustomizeScreenState){CUSTOMIZE_CONFIRM_TIRES, 0, 1, 2};
    screen = CustomizeTickConfirmTimer(&screen);
    EXPECT_EQ(1, screen.confirmTimer);
    screen = CustomizeFinishPopup(&screen);
    EXPECT_EQ(CUSTOMIZE_ACTIVE, screen.phase);
    return 0;
}
