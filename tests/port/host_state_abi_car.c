#include "../../src/port/host_state_car.c"

_Static_assert(sizeof(((GameCarDrive *)0)->roadGrade) == sizeof(s32),
               "per-car road grade must retain its slope width");
_Static_assert(sizeof(g_LaunchSpeedThresholds) ==
                   sizeof(LaunchSpeedThreshold) * CAR_LAUNCH_THRESHOLD_COUNT,
               "g_LaunchSpeedThresholds ABI size changed");
_Static_assert(sizeof(g_TrackPoints) == sizeof(void *),
               "g_TrackPoints must be one pointer");
_Static_assert(sizeof(g_RaceIntroCameraScript) == sizeof(void *),
               "g_RaceIntroCameraScript must be one pointer");
_Static_assert(sizeof(g_FinishCamera.car) == sizeof(GameCarRuntime),
               "finish camera car size changed");
_Static_assert(sizeof(g_FinishCamera.seedYaw) == sizeof(s32),
               "finish camera seed yaw must be one angle");
_Static_assert(sizeof(g_RankedCars) == sizeof(GameCarRuntime *) * 4,
               "g_RankedCars ABI size changed");
_Static_assert(sizeof(g_CarPerformance.torqueBands) == sizeof(s16) * CAR_TORQUE_BAND_COUNT,
               "g_CarPerformance.torqueBands ABI size changed");
_Static_assert(sizeof(g_TrackEventData) == sizeof(void *),
               "g_TrackEventData must be one pointer");
_Static_assert(sizeof(g_CarPerformance.lossBands) ==
                   sizeof(s16) * CAR_TORQUE_BAND_COUNT,
               "g_CarPerformance.lossBands ABI size changed");
_Static_assert(sizeof(g_CarSpec) == sizeof(void *),
               "g_CarSpec must be one pointer");
_Static_assert(sizeof(g_CarPerformance.curves) == sizeof(GearCurveRow) * 7,
               "g_CarPerformance.curves ABI size changed");
