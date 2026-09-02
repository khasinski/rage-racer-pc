#include "../../src/port/host_state_car.c"

_Static_assert(sizeof(g_RoadGrade) == sizeof(s32),
               "g_RoadGrade must be one slope value");
_Static_assert(sizeof(g_LaunchSpeedThresholds) ==
                   sizeof(LaunchSpeedThreshold) * CAR_LAUNCH_THRESHOLD_COUNT,
               "g_LaunchSpeedThresholds ABI size changed");
_Static_assert(sizeof(g_RaceIntroCameraDelta) == sizeof(SVec),
               "g_RaceIntroCameraDelta ABI size changed");
_Static_assert(sizeof(g_TrackPoints) == sizeof(void *),
               "g_TrackPoints must be one pointer");
_Static_assert(sizeof(g_RaceIntroCameraScript) == sizeof(void *),
               "g_RaceIntroCameraScript must be one pointer");
_Static_assert(sizeof(g_CameraCar) == sizeof(GameCarRuntime),
               "g_CameraCar ABI size changed");
_Static_assert(sizeof(g_CameraCarSeedYaw) == sizeof(s32),
               "g_CameraCarSeedYaw must be one angle");
_Static_assert(sizeof(g_RaceIntroCameraCursor) == sizeof(void *),
               "g_RaceIntroCameraCursor must be one pointer");
_Static_assert(sizeof(g_RankedCars) == sizeof(GameCarRuntime *) * 4,
               "g_RankedCars ABI size changed");
_Static_assert(sizeof(g_TorqueBandEnd) == sizeof(s16) * CAR_TORQUE_BAND_COUNT,
               "g_TorqueBandEnd ABI size changed");
_Static_assert(sizeof(g_TrackEventData) == sizeof(void *),
               "g_TrackEventData must be one pointer");
_Static_assert(sizeof(g_TorqueLossBandEnd) ==
                   sizeof(s16) * CAR_TORQUE_BAND_COUNT,
               "g_TorqueLossBandEnd ABI size changed");
_Static_assert(sizeof(g_CarSpec) == sizeof(void *),
               "g_CarSpec must be one pointer");
_Static_assert(sizeof(g_GearTorqueCurve) == sizeof(GearCurveRow) * 7,
               "g_GearTorqueCurve ABI size changed");
