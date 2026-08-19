#include "game/camera_types.h"
#include "game/boot_defaults.h"
#include "game/frontend_state.h"
#include "game/menu_state.h"
#include "game/race_session_state.h"

#define EXPECT_EQ(expected, actual) \
    do { if ((expected) != (actual)) return __LINE__; } while (0)
static void FillBytes(void *destination, u8 value, unsigned long size) {
    u8 *bytes = destination;
    unsigned long i;
    for (i = 0; i < size; i++) bytes[i] = value;
}

static int TestMenuState(void) {
    static u8 emptyScript;
    MenuState expected = MenuStateDefaults(11, &emptyScript);
    MenuState actual;
    MenuVisualState visual;

    EXPECT_EQ(500000, expected.garage.viewAngle);
    EXPECT_EQ(-1, expected.garage.pendingCourseIndex);
    EXPECT_EQ(-1, expected.garage.carSwapToIndex);
    EXPECT_EQ(11, expected.garage.courseModelIndex);
    EXPECT_EQ(1, expected.transition.hintButtonsVisible);
    EXPECT_EQ(&emptyScript, expected.scripts.courseSelectModal);

    FillBytes(&actual, 0xA5, sizeof(actual));
    MenuStateReset(&actual, 11, &emptyScript);
    EXPECT_EQ(expected.garage.viewAngle, actual.garage.viewAngle);
    EXPECT_EQ(expected.garage.pendingCourseIndex, actual.garage.pendingCourseIndex);
    EXPECT_EQ(expected.transition.busy, actual.transition.busy);
    EXPECT_EQ(expected.selection.designModeOption, actual.selection.designModeOption);
    EXPECT_EQ(expected.scripts.engineerShopModal, actual.scripts.engineerShopModal);
    MenuStateReset(&actual, 11, &emptyScript);
    EXPECT_EQ(expected.garage.courseModelIndex, actual.garage.courseModelIndex);

    FillBytes(&visual, 0xA5, sizeof(visual));
    MenuVisualStateReset(&visual);
    EXPECT_EQ(0, visual.courseSelect);
    EXPECT_EQ(0, visual.engineerShop);
    EXPECT_EQ(0, visual.timeAttackPlate);
    return 0;
}

static int TestFrontendState(void) {
    FrontendRuntimeState entry = FrontendStateForEntry();
    FrontendRuntimeState title = FrontendStateForTitle(0);
    FrontendRuntimeState streamTitle = FrontendStateForTitle(1);
    FrontendRuntimeState actual;

    EXPECT_EQ(0x80, entry.frameSyncThreshold);
    EXPECT_EQ(-1, entry.titleAttractTimer);
    EXPECT_EQ(0x1E, title.titleExitTimer);
    EXPECT_EQ(0, title.titleFadeLevel);
    EXPECT_EQ(0xFF, streamTitle.titleFadeLevel);
    EXPECT_EQ(0x190, streamTitle.titleAttractTimer);

    FillBytes(&actual, 0xA5, sizeof(actual));
    FrontendStateResetForEntry(&actual);
    EXPECT_EQ(entry.frameSyncThreshold, actual.frameSyncThreshold);
    EXPECT_EQ(entry.titleAttractTimer, actual.titleAttractTimer);
    FrontendStateResetForEntry(&actual);
    EXPECT_EQ(entry.frontendState, actual.frontendState);
    FrontendStateResetForTitle(&actual, 1);
    EXPECT_EQ(streamTitle.titleFadeLevel, actual.titleFadeLevel);
    EXPECT_EQ(streamTitle.titleAttractTimer, actual.titleAttractTimer);
    return 0;
}

static int TestRaceSessionState(void) {
    RaceSessionState normal = RaceSessionStateDefaults(0, 1000);
    RaceSessionState longRace = RaceSessionStateDefaults(3, 901);
    RaceSessionState actual;

    EXPECT_EQ(3, normal.lapCount);
    EXPECT_EQ(333, normal.sectorEndDistance[0]);
    EXPECT_EQ(666, normal.sectorEndDistance[1]);
    EXPECT_EQ(1000, normal.sectorEndDistance[2]);
    EXPECT_EQ(-2, normal.sectorIndex);
    EXPECT_EQ(CAMERA_VIEW_CAR, normal.cameraViewMode);
    EXPECT_EQ(0x1FE, normal.rivalCueFlags);
    EXPECT_EQ(1, normal.rivalCueEnabled);
    EXPECT_EQ(6, longRace.lapCount);

    FillBytes(&actual, 0xA5, sizeof(actual));
    RaceSessionStateReset(&actual, 3, 901);
    EXPECT_EQ(longRace.lapCount, actual.lapCount);
    EXPECT_EQ(longRace.sectorEndDistance[0], actual.sectorEndDistance[0]);
    EXPECT_EQ(longRace.cameraViewMode, actual.cameraViewMode);
    EXPECT_EQ(longRace.rivalCueFlags, actual.rivalCueFlags);
    RaceSessionStateReset(&actual, 3, 901);
    EXPECT_EQ(longRace.frameSyncThreshold, actual.frameSyncThreshold);
    return 0;
}

static int TestGameBootDefaults(void) {
    GameBootDefaults expected = GameBootDefaultsCreate();
    GameBootDefaults actual;

    EXPECT_EQ(1, expected.input.negconSteerPlay);
    EXPECT_EQ(0x21, expected.padRuntime.validateCountdown);
    FillBytes(&actual, 0xA5, sizeof(actual));
    GameBootDefaultsReset(&actual);
    EXPECT_EQ(expected.display.screenOffsetX, actual.display.screenOffsetX);
    EXPECT_EQ(expected.display.screenOffsetY, actual.display.screenOffsetY);
    EXPECT_EQ(expected.display.mirrorMode, actual.display.mirrorMode);
    EXPECT_EQ(expected.input.negconSteerPlay, actual.input.negconSteerPlay);
    EXPECT_EQ(expected.input.padMappingIndex, actual.input.padMappingIndex);
    EXPECT_EQ(expected.input.negconMappingIndex, actual.input.negconMappingIndex);
    EXPECT_EQ(expected.input.negconSteerNeutral, actual.input.negconSteerNeutral);
    EXPECT_EQ(expected.input.negconNeutralI, actual.input.negconNeutralI);
    EXPECT_EQ(expected.input.negconNeutralII, actual.input.negconNeutralII);
    EXPECT_EQ(expected.input.negconNeutralL, actual.input.negconNeutralL);
    EXPECT_EQ(expected.input.negconMaxTwist, actual.input.negconMaxTwist);
    EXPECT_EQ(expected.padRuntime.errorState, actual.padRuntime.errorState);
    EXPECT_EQ(expected.padRuntime.validateCountdown, actual.padRuntime.validateCountdown);
    EXPECT_EQ(expected.padRuntime.errorHoldBits, actual.padRuntime.errorHoldBits);
    EXPECT_EQ(expected.progress.extraGrandPrixUnlocked,
              actual.progress.extraGrandPrixUnlocked);
    GameBootDefaultsReset(&actual);
    EXPECT_EQ(expected.padRuntime.validateCountdown,
              actual.padRuntime.validateCountdown);
    return 0;
}

int main(void) {
    int result;
    if ((result = TestMenuState()) != 0) return result;
    if ((result = TestFrontendState()) != 0) return result;
    if ((result = TestRaceSessionState()) != 0) return result;
    if ((result = TestGameBootDefaults()) != 0) return result;
    return 0;
}
