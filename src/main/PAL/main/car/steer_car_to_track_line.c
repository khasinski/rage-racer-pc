#include "game/car.h"
#include "game/track.h"
#include "game/render.h"
#include "game/audio.h"
#include "game/random.h"

typedef union EngineRpmAddress {
    s32 *value;
    u16 *rpm;
} EngineRpmAddress;


/*
 * AI route steering: projects a target point ahead of (or behind, per the
 * direction flag at car+0x110) the car on the track centre-line, offset
 * laterally, and steers the car's heading toward it. The heading correction is
 * divided by the spec block's steerResponse. Register pins are
 * match-load-bearing.
 */
void SteerCarToTrackLine(PlayerCarRuntime *car) {
    const GameCarSpec *spec = g_CarSpec;
    s32 lateral = car->trackLateralOffset;
    s32 aheadIndex;
    s32 target[3];
    s32 lineAngle;
    s32 sideways;
    s32 wantedHeading;

    /*
     * Aim two track points along the way the car is going. A car launched
     * backwards counts them the other way, so the point it steers at is still
     * ahead of it.
     */
    aheadIndex = car->drive.launchDirection != 0 ? car->trackPointIndex + 2
                                                 : car->trackPointIndex - 2;
    if (aheadIndex < 0) aheadIndex += g_TrackPointCount;
    aheadIndex %= g_TrackPointCount;

    InterpolateTrackPoint(aheadIndex, target, car->segmentFraction);

    /*
     * Push that point sideways by however far off the racing line this car is
     * meant to run. The line's own angle is negated, which turns the offset
     * from the line's frame into the world's.
     */
    lineAngle = ANGLE_FULL_TURN - SmoothTrackAngle(aheadIndex,
                                                  car->segmentFraction);

    /*
     * Each product is a signed fixed-point value and the shift rounds towards
     * minus infinity, so a negative one is nudged up first to truncate the way
     * a division would. Written out because the bias is part of the position.
     */
    sideways = rsin(lineAngle) * lateral;
    if (sideways < 0) sideways += 0xFFF;
    target[0] += sideways >> 12;

    sideways = rcos(lineAngle) * lateral;
    if (sideways < 0) sideways += 0xFFF;
    target[2] += sideways >> 12;

    wantedHeading = ANGLE_QUARTER_TURN - Atan2(target[0] - car->x,
                                               target[2] - car->z);

    /*
     * A car in the air is not steering. On the ground it turns a fixed share
     * of the way towards the point each frame, and the car's own steering
     * response divides that: a smaller number turns harder.
     *
     * The divisor is truncated to sixteen bits, which is how the recovered
     * code read it, and the response is unsigned, so a value past a signed
     * turn would come back negative and be clamped to one. No car ships with
     * one, and the truncation stays because it is what the game does.
     */
    if (car->verticalMotionState == 0) {
        s32 response = (s16)spec->steerResponse;
        s32 towards;

        if (response <= 0) response = 1;
        towards = GetAngleDelta(car->headingAngle, wantedHeading);
        car->headingAngle += (towards * 20) / response;
    }
}

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
            GameCarSpecAddress specAddress;
            s32 landingRpm;
            s32 gearOffset;
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
            specAddress.pointer = spec;
            specAddress.bytes += drive->gear << 2;
            landingRpm /= specAddress.pointer->gearRatio[0];

            gearOffset = drive->gear << 2;
            RAW(drive->jumpTimer) = 0x14;
            RAW(drive->motionState) = CAR_MOTION_AIRBORNE;
            g_ShiftTargetRpm = landingRpm;
            drive->shiftRpmDelta =
                (s16)(g_ShiftTargetRpm - (u16)drive->engineRpm);

            specAddress.pointer = spec;
            specAddress.bytes += gearOffset;
            drive->engineLoad =
                landingRpm * specAddress.pointer->gearLoad[0] / 0x20000;
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
    {
        s32 launched = car->headingAngle;
        AdvanceCarPosition(AsRivalCar(car));
        car->headingAngle = launched;
    }

    drive->accelPos = rsin(car->headingAngle) * car->speed / 256;
    drive->brakePos = rcos(car->headingAngle) * car->speed / 256;
}

/*
 * Car motion handler for motionState == CAR_MOTION_AIRBORNE (airborne / jump): decays velocity and
 * spin, advances the car (AdvanceCarPosition), and lands it when it returns to the
 * ground. The drive sub-block is the GameCarDrive view beginning at +0xBC.
 */
void UpdateCarAirborne(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 bodySin;
    s32 bodyCos;
    s32 alongBody;

    /*
     * A car in the air keeps the tyre voice going, pitched by how far it is
     * turned off its axis, unless it is landing on a gearchange, which has a
     * note of its own.
     */
    if (g_ShiftSoundLevel == 0) {
        s32 offAxis = drive->yawOffset;
        s32 phase = offAxis < 513 ? offAxis * 3 + 6144 : 0x1E00;

        SetIndexedEffectVoice(0, phase, drive->jumpTimer * 2 + 80);
    } else {
        SetIndexedEffectVoice(0, 0x1800, g_ShiftSoundLevel + 25);
    }

    /* The body settles a fifth of the way towards where it should point. */
    car->bodyYaw += GetAngleDelta(car->bodyYaw, drive->targetHeading) / 5;
    AdvanceCarPosition(AsRivalCar(car));

    bodySin = rsin(car->bodyYaw);
    bodyCos = rcos(car->bodyYaw);

    /*
     * Momentum is carried in the world, but a car in the air is turned away
     * from it. Take the velocity into the body's frame, keep the part along
     * the body, and put it back: that is the drift a jumping car has, and it
     * is why landing sideways throws the car off line rather than stopping.
     *
     * The sideways part was worked out too and never read, so it is gone.
     */
    drive->accelPos =
        rsin(car->headingAngle + drive->yawOffset) * car->speed / 256;
    drive->brakePos =
        rcos(car->headingAngle + drive->yawOffset) * car->speed / 256;

    alongBody = (bodySin * drive->accelPos + bodyCos * drive->brakePos) / 4096;

    drive->accelPos = rsin(drive->launchHeading) * drive->launchSpeed / 256 +
                      bodySin * alongBody / 4096;
    drive->brakePos = rcos(drive->launchHeading) * drive->launchSpeed / 256 +
                      bodyCos * alongBody / 4096;

    /* Frames spent coasting, which the landing reads to decide its grip. */
    if (drive->acceleratorLatch != 1 && drive->brakeLatch != 1 &&
        drive->acceleratorInput.value < 128) {
        drive->groundedFrames += 1;
    } else {
        drive->groundedFrames = 0;
    }

    /* Everything the jump gave the car bleeds away at the same rate. */
    drive->spinRate = drive->spinRate * 31 / 32;
    drive->launchSpeed = drive->launchSpeed * 31 / 32;
    drive->yawOffset = drive->yawOffset * 31 / 32;
    drive->bodyLiftOffset = drive->bodyLiftOffset * 2 / 3;

    /* Still badly sideways: the air costs it speed. */
    if (drive->yawOffset >= 1537) {
        car->speed = car->speed * 4 / 5;
    }

    if (drive->jumpTimer <= 0) {
        SetIndexedEffectVoice(-1, 0, 0);
        car->bodyYaw -= drive->spinRate;
        g_ShiftSoundLevel = 0;
        drive->shiftRpmDelta = 0;
        drive->yawOffset = 0;
        drive->launchSpeed = 0;
        drive->motionState = CAR_MOTION_DRIVING;
        drive->bodyLiftOffset = 0;
    }
}

void UpdateCarStandingStart(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 bodySin;
    s32 bodyCos;
    s32 alongBody;

    /* The body settles a fifth of the way towards where it should point. */
    car->bodyYaw += GetAngleDelta(car->bodyYaw, drive->targetHeading) / 5;
    AdvanceCarPosition(AsRivalCar(car));

    bodySin = rsin(car->bodyYaw);
    bodyCos = rcos(car->bodyYaw);

    /*
     * Only the part of the motion that runs along the body survives: a car
     * spinning its wheels on the line goes where it points, not where it was
     * drifting, and at a sixteenth of the scale the rest of the game uses.
     */
    drive->accelPos = rsin(car->headingAngle) * car->speed / 256;
    drive->brakePos = rcos(car->headingAngle) * car->speed / 256;
    alongBody = (bodySin * drive->accelPos + bodyCos * drive->brakePos) / 4096;
    drive->accelPos = bodySin * alongBody / 16384;
    drive->brakePos = bodyCos * alongBody / 16384;

    /* The wheelspin is heard rising and falling with the throttle. */
    SetIndexedEffectVoice(0, 0x1A80,
                          (0x60 - (g_StandingStartSpin & 0x1F) * 2) *
                              drive->acceleratorInput.value / 256);

    /* Almost none of the speed reaches the road while the wheels are spinning. */
    car->speed = car->speed / 10;

    if (g_StandingStartSpin >= 11) {
        s32 throttle = drive->acceleratorInput.value;
        s32 rpm = drive->engineRpm;
        s32 grip;

        /*
         * How fast the wheels stop spinning. Off the power band they hook up
         * almost at once; on it, a light throttle still finds grip sooner than
         * a heavy one. Braking cuts the spin as well.
         */
        grip = throttle + 32;
        g_StandingStartSpin -= drive->brakeInput * 2;
        if (rpm < 2000) grip = throttle + 1032;
        if (throttle < 127 && rpm >= 2001) grip += 127;

        /* The car shakes while they spin, harder the harder they are gripping. */
        drive->standingStartBounceY = (Random15() & 3) * grip / 256;
        drive->standingStartBounceX = (Random15() & 7) * grip / 256;

        g_StandingStartSpin -= grip;
        if (g_StandingStartSpin <= 0) {
            drive->standingStartBounceY = 0;
            drive->standingStartBounceX = 0;
            drive->motionState = CAR_MOTION_DRIVING;
            SetIndexedEffectVoice(-1, 0, 0);
        }
    } else {
        SetIndexedEffectVoice(-1, 0, 0);
        drive->motionState = CAR_MOTION_DRIVING;
        drive->standingStartBounceY = 0;
        drive->standingStartBounceX = 0;
    }
}

/*
 * Finds the track segment whose (rotated, half-width) quad currently contains
 * the car. Starting at `idx` it spirals outward over neighbouring segments
 * (k alternately added/subtracted), and for each builds the segment quad from
 * the two endpoints' angle plus their left/right half-widths and
 * runs four half-plane cross-product tests (NormalClip). Returns the
 * containing segment index, or -1 (snapping the car onto the track) if none.
 * pts[0] is the car-relative point; pts[1..4] are the quad corners.
 */
s32 FindTrackSegment(GameCarRuntime *car, s32 idx) {
    DVecValue pts[5];
    s32 i;
    s32 k;
    s32 nxt;
    s32 ni;
    s32 carx;
    s32 carz;
    s32 sx;
    s32 sz;
    s32 cos_c;
    s32 sin_c;
    s32 cos_n;
    s32 sin_n;
    s32 pax;
    s32 paz;
    s32 f10a;
    s32 f12a;
    s32 f10b;
    s32 f12b;
    GameTrackPoint *pa;
    GameTrackPoint *pb;

    k = 0;
    carx = car->x;
    carz = car->z;
    i = idx;

    do {
        nxt = (i + 1) % g_TrackPointCount;
        pa = TrackPoint(i);
        pb = TrackPoint(nxt);

        pax = pa->x;
        paz = pa->z;
        sx = pb->x - pax;
        sz = pb->z - paz;
        pts[0].components.vx = carx - pax;
        pts[0].components.vy = carz - paz;

        cos_c = rcos(0xC00 - pa->angle);
        sin_c = rsin(0xC00 - pa->angle);
        cos_n = rcos(0xC00 - pb->angle);
        sin_n = rsin(0xC00 - pb->angle);

        f10a = pa->leftHalfWidth;
        f12a = pa->rightHalfWidth;
        f12b = pb->rightHalfWidth;
        f10b = pb->leftHalfWidth;

        pts[1].components.vx =  (s16)(f10a * 2) * (s16)cos_c / 4096;
        pts[1].components.vy = -(s16)(f10a * 2) * (s16)sin_c / 4096;
        pts[2].components.vx = -(s16)(f12a * 2) * (s16)cos_c / 4096;
        pts[2].components.vy =  (s16)(f12a * 2) * (s16)sin_c / 4096;
        pts[3].components.vx = sx + (s16)(f10b * 2) * (s16)cos_n / 4096;
        pts[3].components.vy = sz - (s16)(f10b * 2) * (s16)sin_n / 4096;
        pts[4].components.vx = sx - (s16)(f12b * 2) * (s16)cos_n / 4096;
        pts[4].components.vy = sz + (s16)(f12b * 2) * (s16)sin_n / 4096;

        if (NormalClip(pts[1].packed, pts[2].packed, pts[0].packed) >= 0 &&
            NormalClip(pts[2].packed, pts[4].packed, pts[0].packed) >= 0 &&
            NormalClip(pts[4].packed, pts[3].packed, pts[0].packed) > 0 &&
            NormalClip(pts[3].packed, pts[1].packed, pts[0].packed) >= 0) {
            return i;
        }

        k++;
        if (k % 2) {
            i += k;
        } else {
            i -= k;
        }
        if (i >= 0) {
            ni = i % g_TrackPointCount;
        } else {
            ni = (i + g_TrackPointCount) % g_TrackPointCount;
        }
        i = ni;
    } while (i != idx);

    car->x = TrackPoint(i)->x;
    car->z = TrackPoint(i)->z;
    return -1;
}
