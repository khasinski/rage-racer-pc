#include "port/mp_client.h"
#include <stdio.h>
#include <string.h>
#define CHECK(test) do { if (!(test)) { fprintf(stderr, "line %d: %s\n", __LINE__, #test); return 1; } } while (0)

int main(void) {
    const GameTrackPoint points[3] = {0};
    RaceSim source = {.route = {.points = points, .count = 3, .length = 3000},
        .laps = 1, .phase = SIM_RACING, .tick = 100, .elapsed = 80};
    for (int seat = 0; seat < MP_FIELD_LIMIT; ++seat) source.drivers[seat].variant = -1;
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        source.drivers[seat].status = SIM_DRIVING;
        source.drivers[seat].car.drive.gear = 1;
    }
    const RaceSim original = source;
    MpCommands commands = {0};
    MpCommand pending[2] = {{.sequence = 1, .tick = 101, .input = {.throttle = 128}},
                            {.tick = 102, .input = {.brake = 64}}};
    RaceSim output;
    CHECK(MpPredictRace(&source, &commands, pending, 2, 1, 100, &output));
    CHECK(memcmp(&output, &source, sizeof(output)) == 0);
    for (int bad = 0; bad < 9; ++bad) {
        MpCommand invalid[2] = {pending[0], pending[1]};
        unsigned count = 2; int seat = 1; uint32_t target = 100;
        if (bad == 0) count = 3;
        if (bad == 1) seat = 2;
        if (bad == 2) target = 99;
        if (bad == 3) target = 111;
        if (bad == 4) invalid[1].input.throttle = 257;
        if (bad == 5) invalid[1].tick = 100;
        if (bad == 6) invalid[0].sequence = 2;
        if (bad == 7) invalid[1].sequence = 2;
        if (bad == 8) commands.count = 1;
        CHECK(!MpPredictRace(&source, &commands, invalid, count, seat, target, &output));
        CHECK(memcmp(&source, &original, sizeof(source)) == 0);
        CHECK(memcmp(&output, &original, sizeof(output)) == 0);
        commands = (MpCommands){0};
    }
    CHECK(!MpPredictRace(&source, &commands, pending, 2, 1, 100, &source));
    CHECK(!MpPredictRace(NULL, &commands, pending, 2, 1, 100, &output));
    CHECK(!MpPredictRace(&source, NULL, pending, 2, 1, 100, &output));
    CHECK(!MpPredictRace(&source, &commands, NULL, 2, 1, 100, &output));
    CHECK(!MpPredictRace(&source, &commands, pending, 2, 1, 100, NULL));
    source.drivers[1].status = SIM_EMPTY;
    CHECK(!MpPredictRace(&source, &commands, NULL, 0, 1, 100, &output));
    source = original;
    source.drivers[1].status = SIM_DRIVER_FINISHED;
    source.drivers[1].place = 1;
    source.drivers[1].car.activeFlag = -1;
    source.finishCount = 1;
    CHECK(MpPredictRace(&source, &commands, NULL, 0, 1, 110, &output));
    CHECK(memcmp(&output, &source, sizeof(output)) == 0);
    source = original;
    source.drivers[1].status = SIM_RETIRED;
    source.drivers[1].car.activeFlag = -1;
    CHECK(MpPredictRace(&source, &commands, NULL, 0, 1, 110, &output));
    CHECK(memcmp(&output, &source, sizeof(output)) == 0);
    source = original;
    source.phase = SIM_FINISHED;
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        source.drivers[seat].status = SIM_RETIRED;
        source.drivers[seat].car.activeFlag = -1;
    }
    CHECK(MpPredictRace(&source, &commands, NULL, 0, 1, 110, &output));
    CHECK(memcmp(&output, &source, sizeof(output)) == 0);
    RaceSim configured = original;
    configured.phase = SIM_SETUP; configured.tick = configured.elapsed = 0;
    configured.drivers[0].variant = 0; configured.drivers[1].variant = 1;
    MpCarConfig config = {.variant = {0, 1}};
    config.specs[0].torqueCurve[0] = 2000;
    config.specs[1].torqueCurve[0] = 4000;
    RaceSim before = configured;
    config.variant[1] = 2;
    CHECK(!MpApplyConfig(&configured, &config));
    CHECK(memcmp(&configured, &before, sizeof(before)) == 0);
    config.variant[1] = 1;
    CHECK(MpApplyConfig(&configured, &config));
    CHECK(configured.drivers[0].engine.curves[0].values[0] == 100);
    CHECK(configured.drivers[1].engine.curves[0].values[0] == 200);
    CHECK(configured.drivers[0].car.x == before.drivers[0].car.x);
    CHECK(configured.drivers[1].random == before.drivers[1].random);
    CHECK(!MpApplyConfig(NULL, &config));
    CHECK(!MpApplyConfig(&configured, NULL));
    configured.phase = SIM_COUNTDOWN;
    before = configured;
    CHECK(!MpApplyConfig(&configured, &config));
    CHECK(memcmp(&configured, &before, sizeof(before)) == 0);
    SimDriver earlier = original.drivers[0], later = earlier;
    earlier.car.x = 100; later.car.x = 300;
    earlier.car.bodyYaw = 4090; later.car.bodyYaw = 6;
    later.car.drive.brakeInput = 128;
    PlayerCarRuntime pose;
    CHECK(MpBlendDriver(&earlier, &later, 32768, &pose));
    CHECK(pose.x == 200 && pose.bodyYaw == 0 && pose.drive.brakeInput == 128);
    CHECK(MpBlendDriver(&earlier, &later, 0, &pose) && pose.x == 100);
    CHECK(MpBlendDriver(&earlier, &later, 65536, &pose) && pose.x == 300);
    PlayerCarRuntime preserved = pose;
    CHECK(!MpBlendDriver(&earlier, &later, 65537, &pose));
    CHECK(memcmp(&pose, &preserved, sizeof(pose)) == 0);
    later.variant = 1;
    CHECK(!MpBlendDriver(&earlier, &later, 32768, &pose));
    CHECK(memcmp(&pose, &preserved, sizeof(pose)) == 0);
    later.variant = earlier.variant;
    later.rival = 1;
    CHECK(!MpBlendDriver(&earlier, &later, 32768, &pose));
    later.rival = 0;
    later.status = SIM_DRIVER_FINISHED; later.place = 1; later.car.activeFlag = -1;
    CHECK(MpBlendDriver(&earlier, &later, 32768, &pose));
    CHECK(memcmp(&pose, &later.car, sizeof(pose)) == 0);
    CHECK(!MpBlendDriver(NULL, &later, 32768, &pose));
    CHECK(!MpBlendDriver(&earlier, NULL, 32768, &pose));
    CHECK(!MpBlendDriver(&earlier, &later, 32768, NULL));
    CHECK(earlier.car.x == 100 && later.car.x == 300);
    RaceSim motion = original;
    uint32_t nextTick = 123, blend = 456;
    CHECK(MpMotionTarget(&motion, 100, 0, 32768, &nextTick, &blend));
    CHECK(nextTick == 102 && blend == 16384);
    motion.tick = 101; motion.elapsed = 81;
    CHECK(MpMotionTarget(&motion, 100, 0, 0, &nextTick, &blend));
    CHECK(nextTick == 102 && blend == 32768);
    CHECK(MpMotionTarget(&motion, 100, 0, 65535, &nextTick, &blend));
    CHECK(nextTick == 102 && blend == 65535);
    CHECK(MpMotionTarget(&motion, 91, 0, 32768, &nextTick, &blend));
    CHECK(nextTick == 101 && blend == 0);
    motion.tick = UINT32_MAX; motion.elapsed = 81;
    CHECK(MpMotionTarget(&motion, UINT32_MAX - 5, 0, 65535, &nextTick, &blend));
    CHECK(nextTick == UINT32_MAX && blend == 0);
    motion = original; motion.phase = SIM_COUNTDOWN; motion.elapsed = 0;
    CHECK(MpMotionTarget(&motion, 100, 0, 32768, &nextTick, &blend));
    CHECK(nextTick == 101 && blend == 32768);
    motion.drivers[0].status = SIM_RETIRED;
    CHECK(MpMotionTarget(&motion, 100, 0, 32768, &nextTick, &blend));
    CHECK(nextTick == 100 && blend == 0);
    motion = original;
    for (int invalid = 0; invalid < 5; ++invalid) {
        motion = original;
        uint32_t authority = 100, clockFraction = 0;
        if (invalid == 0) authority = 101;
        if (invalid == 1) authority = 89;
        if (invalid == 2) clockFraction = 65536;
        if (invalid == 3) motion.drivers[0].rival = 1;
        if (invalid == 4) motion.phase = SIM_SETUP;
        nextTick = 123; blend = 456;
        CHECK(!MpMotionTarget(&motion, authority, 0, clockFraction, &nextTick, &blend));
        CHECK(nextTick == 123 && blend == 456);
    }
    puts("mp_prediction: bounded scratch ownership, invalid samples and terminal state pass");
    return 0;
}
