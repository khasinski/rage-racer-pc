#include "game/camera_types.h"
#include "game/frontend_state.h"
#include "game/menu_state.h"
#include "game/persistent_settings.h"
#include "game/race_session_state.h"

#define EXPECT_EQ(expected, actual) \
    do { if ((expected) != (actual)) return __LINE__; } while (0)
#define EXPECT_SAME(expected, actual) \
    do { if (!BytesEqual(&(expected), &(actual), sizeof(expected))) return __LINE__; } while (0)

static void FillBytes(void *destination, u8 value, unsigned long size) {
    u8 *bytes = destination;
    unsigned long i;
    for (i = 0; i < size; i++) bytes[i] = value;
}

static int BytesEqual(const void *lhs, const void *rhs, unsigned long size) {
    const u8 *left = lhs;
    const u8 *right = rhs;
    unsigned long i;
    for (i = 0; i < size; i++) {
        if (left[i] != right[i]) return 0;
    }
    return 1;
}

static int TestMenuState(void) {
    static u8 emptyScript;
    MenuState expected = MenuStateDefaults(11, &emptyScript);
    MenuState actual;
    MenuVisualState visual;

    EXPECT_EQ(500000, expected.viewAngle);
    EXPECT_EQ(-1, expected.pendingCourseIndex);
    EXPECT_EQ(-1, expected.carSwapToIndex);
    EXPECT_EQ(11, expected.courseModelIndex);
    EXPECT_EQ(1, expected.hintButtonsVisible);
    EXPECT_EQ(&emptyScript, expected.emptyScript);

    FillBytes(&actual, 0xA5, sizeof(actual));
    MenuStateReset(&actual, 11, &emptyScript);
    EXPECT_SAME(expected, actual);
    MenuStateReset(&actual, 11, &emptyScript);
    EXPECT_SAME(expected, actual);

    FillBytes(&visual, 0xA5, sizeof(visual));
    MenuVisualStateReset(&visual);
    EXPECT_EQ(1, BytesEqual(&visual, &expected.visual, sizeof(visual)));
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
    EXPECT_SAME(entry, actual);
    FrontendStateResetForEntry(&actual);
    EXPECT_SAME(entry, actual);
    FrontendStateResetForTitle(&actual, 1);
    EXPECT_SAME(streamTitle, actual);
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
    EXPECT_SAME(longRace, actual);
    RaceSessionStateReset(&actual, 3, 901);
    EXPECT_SAME(longRace, actual);
    return 0;
}

static int TestPersistentSettings(void) {
    PersistentSettings expected = PersistentSettingsDefaults();
    PersistentSettings actual;

    EXPECT_EQ(1, expected.negconSteerPlay);
    EXPECT_EQ(0x21, expected.padValidateCountdown);
    FillBytes(&actual, 0xA5, sizeof(actual));
    PersistentSettingsReset(&actual);
    EXPECT_EQ(expected.screenOffsetX, actual.screenOffsetX);
    EXPECT_EQ(expected.screenOffsetY, actual.screenOffsetY);
    EXPECT_EQ(expected.negconSteerPlay, actual.negconSteerPlay);
    EXPECT_EQ(expected.padMappingIndex, actual.padMappingIndex);
    EXPECT_EQ(expected.negconMappingIndex, actual.negconMappingIndex);
    EXPECT_EQ(expected.negconSteerNeutral, actual.negconSteerNeutral);
    EXPECT_EQ(expected.negconNeutralI, actual.negconNeutralI);
    EXPECT_EQ(expected.negconNeutralII, actual.negconNeutralII);
    EXPECT_EQ(expected.negconNeutralL, actual.negconNeutralL);
    EXPECT_EQ(expected.negconMaxTwist, actual.negconMaxTwist);
    EXPECT_EQ(expected.padErrorState, actual.padErrorState);
    EXPECT_EQ(expected.padValidateCountdown, actual.padValidateCountdown);
    EXPECT_EQ(expected.padErrorHoldBits, actual.padErrorHoldBits);
    EXPECT_EQ(expected.mirrorMode, actual.mirrorMode);
    EXPECT_EQ(expected.extraGrandPrixUnlocked, actual.extraGrandPrixUnlocked);
    PersistentSettingsReset(&actual);
    EXPECT_EQ(expected.padValidateCountdown, actual.padValidateCountdown);
    EXPECT_EQ(expected.extraGrandPrixUnlocked, actual.extraGrandPrixUnlocked);
    return 0;
}

int main(void) {
    int result;
    if ((result = TestMenuState()) != 0) return result;
    if ((result = TestFrontendState()) != 0) return result;
    if ((result = TestRaceSessionState()) != 0) return result;
    if ((result = TestPersistentSettings()) != 0) return result;
    return 0;
}
