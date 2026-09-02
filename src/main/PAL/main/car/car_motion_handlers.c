#include "game/angle.h"
#include "game/audio.h"
#include "game/car.h"
#include "psyq/gte.h"

/*
 * Car motion-state handler for motionState == CAR_MOTION_TAKEOFF: the one-frame takeoff of a jump.
 * Turns the launch spin UpdateCarDriving seeded into clamped yaw, recomputes revs /
 * tacho / world velocity, then sets route+0x38 = 0x14 and route+0x98 = 2 to hand
 * the car to the airborne handler UpdateCarAirborne.
 */


void UpdateCarLaunch(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 startYaw = car->bodyYaw;
    s32 startHeading = car->headingAngle;
    s32 spin = drive->spinRate;
    s32 spinMagnitude = spin < 0 ? -spin : spin;
    s32 skid;

    /*
     * How far the car is sideways: the angle between the way it faces and the
     * way it travels. Everything here is decided by it. Past a certain slide
     * the tyres scrub speed off.
     */
    skid = GetAngleDistance(startYaw, startHeading);
    if (skid >= 0x600) {
        car->speed = car->speed * 990 / 1000;
    }

    /*
     * The scrub is what the player hears: a car still on the ground pitches
     * and swells its tyre voice with the slide, and one in the air silences
     * it.
     */
    if (car->verticalMotionState == 0) {
        int sliding = skid < 513;
        s32 volume = sliding ? skid / 8 + 0x40 : 0x7F;
        s32 phase = sliding ? skid * 3 + 0x1800 : 0x1E00;

        SetIndexedEffectVoice(0, phase, volume);
    } else {
        SetIndexedEffectVoice(-1, 0, 0);
    }

    /*
     * A launch has a budget. Spinning slowly and driving slowly both cost it
     * before anything else happens, and when it runs out the car settles.
     */
    if (skid < 0x80 && spinMagnitude < 0x800) {
        drive->launchEnergy -= (0x800 - spinMagnitude) * 4000 / 256;
    }
    if (car->speed < 0x190) {
        drive->launchEnergy -= (0x190 - car->speed) * 100;
    }

    if (drive->launchEnergy > 0) {
        s32 towardsTarget;

        /* The body rises to its full lift over ten frames and stays there. */
        drive->bodyLiftOffset += 10;
        if (drive->bodyLiftOffset >= 100) {
            drive->bodyLiftOffset = 100;
        }

        /*
         * The spin is fed by how far the body still has to turn to reach the
         * heading it was launched at, weighted by how loaded the steering was.
         */
        towardsTarget = GetAngleDelta(car->bodyYaw, drive->targetHeading) * 98 / 100;
        towardsTarget = towardsTarget * (drive->steeringLoadAngle + 0x800) / 2048;
        drive->spinRate += towardsTarget * 16;

        /*
         * With the wheels near straight the car recovers: an almost-aligned
         * body has its spin bled off and pulled towards the heading, and a
         * slower spin is helped along instead.
         */
        if ((u32)(drive->steerPos + 127) < 255) {
            if (GetAngleDistance(car->bodyYaw, car->headingAngle) < 0x200) {
                drive->spinRate = drive->spinRate * 31 / 32;
                drive->spinRate += GetAngleDelta(car->bodyYaw, car->headingAngle);
            } else if (spinMagnitude < 0x800) {
                drive->spinRate += towardsTarget / 2;
            }
        }

        if (drive->spinRate > 0x3600) drive->spinRate = 0x3600;
        if (drive->spinRate < -0x3600) drive->spinRate = -0x3600;

        car->bodyYaw += drive->spinRate / 256;

        /*
         * What the frame costs. Sliding costs by the square of the angle, a
         * slow spin costs what it is short of the limit, and going gently or
         * braking costs on top.
         */
        drive->launchEnergy -= 64;
        skid = GetAngleDistance(car->bodyYaw, car->headingAngle);
        drive->launchEnergy -= skid * skid / 65536;
        drive->launchEnergy -= (0x3600 - spinMagnitude) / 64;
        if (car->speed < drive->speedScale / 2) {
            drive->launchEnergy -= (drive->speedScale / 2 - car->speed) / 8;
        }
        drive->launchEnergy -= drive->brakeInput * 4;
        drive->launchEnergy -= (0x100 - drive->acceleratorInput.value) * 4;

        car->speed -= drive->brakeInput * 10 / 256;
        car->speed -= (0x100 - drive->acceleratorInput.value) * 10 / 256;
    } else {
        drive->spinRate = drive->spinRate * 15 / 16;

        /*
         * The budget is gone. A car not spinning wildly lands here: the
         * heading snaps to the body and the drivetrain is recomputed for the
         * gear it will land in, before the airborne handler takes over. One
         * still spinning hard just keeps winding down.
         */
        if (spinMagnitude < 0x1000) {
            GameCarSpec *spec = g_CarSpec;
            s32 landingRpm;
            s32 offAxis;
            u32 shiftRpmRange;

            drive->drivetrainTorque =
                (100 - (drive->gear - 1) * 4) * 10000 * car->speed / 100;
            drive->yawOffset = GetAngleDelta(car->headingAngle, car->bodyYaw);
            drive->launchHeading = car->headingAngle;
            car->headingAngle = car->bodyYaw;

            /*
             * How much speed survives the landing: the square of how far the
             * car is off its own axis, folded the other way past a quarter
             * turn, so landing backwards keeps as little as landing sideways.
             */
            offAxis = drive->yawOffset < 0 ? -drive->yawOffset : drive->yawOffset;
            offAxis = offAxis < 0x401 ? offAxis * offAxis
                                      : (0x800 - offAxis) * (0x800 - offAxis);
            drive->launchSpeed = offAxis * car->speed / 0x100000;
            drive->spinRate = 0;

            landingRpm = car->speed * 0xA0 / 1168 * 10000;
            landingRpm /= spec->gearRatio[drive->gear];

            drive->jumpTimer = 0x14;
            drive->motionState = CAR_MOTION_AIRBORNE;
            g_ShiftTargetRpm = landingRpm;
            drive->shiftRpmDelta =
                (s16)(g_ShiftTargetRpm - (u16)drive->engineRpm);

            drive->engineLoad =
                landingRpm * spec->gearLoad[drive->gear] / 0x20000;
            if (drive->manual == 0) {
                drive->engineLoad = drive->engineLoad * 985 / 1000;
            }

            /* The shift sound plays only when the landing revs come out near
             * the revs the engine is already turning. */
            shiftRpmRange = ((u16)drive->shiftRpmDelta + 99) & 0xFFFF;
            g_ShiftSoundLevel = shiftRpmRange < 199;
        }
    }

    drive->targetHeading = car->bodyYaw;
    SteerCarToTrackLine(car);

    /*
     * Acceleration reaching the road falls away with the slide: past a
     * quarter turn only a fraction of it does anything, and below that it
     * tapers to nothing as the car comes straight.
     */
    skid = GetAngleDistance(car->bodyYaw, car->headingAngle);
    if (skid >= 0x401) {
        s32 push = car->acceleration;
        s32 scaled = (0x3600 - spinMagnitude) * 4 * push * (skid - 0x400);

        car->acceleration = push / 2 + scaled / 14155776;
    } else {
        car->acceleration = (0x200 - skid) * car->acceleration / 512;
    }

    /*
     * The shared advance decides a heading from the motion; a launching car
     * keeps the one its spin gave it and takes only the speed.
     */
    s32 launchedHeading = car->headingAngle;

    AdvanceCarPosition(AsRivalCar(car));
    car->headingAngle = launchedHeading;

    drive->accelPos = rsin(car->headingAngle) * car->speed / 256;
    drive->brakePos = rcos(car->headingAngle) * car->speed / 256;
}
