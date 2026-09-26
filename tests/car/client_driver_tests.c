#include "driver_fixture.h"
#include "game/car_internal.h"
#include "game/track_internal.h"
#include "game/race.h"
#include "game/state.h"
#include "game/random.h"
#include <stdio.h>
#include <string.h>


GameCarSpec *g_CarSpec;
CarPerformance g_CarPerformance;
const GameTrackPoint *g_TrackPoints;
const GameTrackArcCenter *g_TrackArcCenters;
const TrackEventData *g_TrackEventData;
s32 g_TrackPointCount, g_TrackLength, g_RaceSeries;
s16 g_RacePhase;
u8 g_PadType;
u32 g_RandomSeed;
LaunchSpeedThreshold g_LaunchSpeedThresholds[CAR_LAUNCH_THRESHOLD_COUNT];
static DriverInput input;
DriverInput ReadDriverInput(void) { return input; }
s32 CollidePlayerWithCars(PlayerCarRuntime *car) { (void)car; return 0; }
void PlayCarDrivingVoice(const PlayerCarRuntime *car, const GameCarSpec *spec) { (void)car; (void)spec; }
void PlayCarLaunchVoice(const PlayerCarRuntime *car) { (void)car; }
void PlayCarAirborneVoice(const PlayerCarRuntime *car) { (void)car; }
void PlayCarStandingStartVoice(const PlayerCarRuntime *car) { (void)car; }
void SetIndexedEffectVoice(s32 index, s32 phase, s32 volume) { (void)index; (void)phase; (void)volume; }
void PlayPlayerLandingCue(s32 frames, int audible) { (void)frames; (void)audible; }
void PlayPlayerContactCue(const PlayerCarRuntime *car, s32 skid, s32 slip, int audible) { (void)car; (void)skid; (void)slip; (void)audible; }
void UpdatePlayerEnginePresentation(const PlayerCarRuntime *car, const GameCarSpec *spec, int finished) { (void)car; (void)spec; (void)finished; }

int main(void) {
    const GameTrackPoint points[3] = {
        {.segmentLength = 1000, .leftHalfWidth = 300, .rightHalfWidth = 300},
        {.x = 1000, .segmentLength = 1000, .leftHalfWidth = 300, .rightHalfWidth = 300},
        {.x = 2000, .segmentLength = 1000, .leftHalfWidth = 300, .rightHalfWidth = 300},
    };
    const TrackRoute route = {.points = points, .count = 3, .length = 3000};
    g_TrackPoints = points; g_TrackPointCount = 3; g_TrackLength = 3000;
    g_LaunchSpeedThresholds[0] = (LaunchSpeedThreshold){960, 320};
    for (int reverse = 0; reverse < 2; reverse++) {
        for (int analog = 0; analog < 2; analog++) {
            for (int manual = 0; manual < 2; manual++) {
                for (int started = 0; started < 2; started++) {
                    PlayerCarRuntime client, headless;
                    GameCarSpec spec;
                    PrepareDriver(&client, &spec, &g_CarPerformance);
                    const TrackRivalStart position = {.x = 200};
                    const DriverStart start = {.route = &route, .position = &position,
                        .reverse = reverse, .manual = manual, .modelIndex = 0};
                    InitDriver(&client, &spec, &g_CarPerformance, &start);
                    headless = client;
                    g_CarSpec = &spec; g_RaceSeries = reverse;
                    g_PadType = analog ? PAD_TYPE_NEGCON : PAD_TYPE_DIGITAL;
                    g_RacePhase = started ? RACE_PHASE_ACTIVE : RACE_PHASE_COUNTDOWN;
                    g_RandomSeed = 123;
                    u32 random = g_RandomSeed;
                    const DriverContext context = {.spec = &spec, .performance = &g_CarPerformance,
                        .route = &route, .corners = g_CarCornerOffsets,
                        .launchThreshold = &g_LaunchSpeedThresholds[0], .reverse = reverse,
                        .analogSteering = analog, .drive = {.started = started, .racing = started,
                            .digitalSteering = !analog}};
                    for (int tick = 0; tick < 40; tick++) {
                        input = (DriverInput){.throttle = 256, .brake = tick > 30 ? 128 : 0,
                            .steering = {.mode = analog ? STEERING_ANALOG : STEERING_DIGITAL,
                                .angle = tick & 1 ? 256 : -256, .left = tick & 1,
                                .right = !(tick & 1)}, .shiftUp = tick == 15};
                        UpdatePlayerCar(&client);
                        DriverStep step = MoveDriver(&headless, &input, &context, &random);
                        FinishDriver(&headless, &context, &random, 0, &step);
                        if (memcmp(&client, &headless, sizeof(client)) != 0 || g_RandomSeed != random) {
                            fprintf(stderr, "driver mismatch: reverse=%d analog=%d manual=%d started=%d tick=%d\n",
                                    reverse, analog, manual, started, tick);
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return 0;
}
