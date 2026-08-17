#include "common.h"
#include "game/vector.h"
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
    GameCarSpec *spec;
    s32 timer;
    s32 index;
    register s32 lateral asm("$18");
    register s32 baseIndex asm("$4");
    s32 finalAngle;
    s32 coords[3];
    s32 angle;
    s32 value;
    s32 xValue;
    s32 headingDelta;
    s32 rawIndex;
    s32 trackCount;
    s32 directionFlag;
    s32 divisor;

    spec = g_CarSpec;
    lateral = car->trackLateralOffset;
    timer = spec->steerResponse;
    directionFlag = car->drive.launchDirection;

    asm volatile("" : "=r"(timer) : "0"(timer));
    baseIndex = car->trackPointIndex;
    index = baseIndex + 2;
    if (directionFlag == 0) {
        index = baseIndex - 2;
    }

    rawIndex = index;
    if (index < 0) {
        rawIndex = index + g_TrackPointCount;
    }
    trackCount = g_TrackPointCount;
    index = rawIndex % trackCount;

    InterpolateTrackPoint(index, coords, car->segmentFraction);
    angle = 0x1000 - SmoothTrackAngle(index, car->segmentFraction);

    xValue = rsin(angle) * lateral;
    if (xValue < 0) {
        xValue += 0xFFF;
    }
    coords[0] += xValue >> 12;

    value = rcos(angle) * lateral;
    if (value < 0) {
        value += 0xFFF;
    }
    coords[2] += value >> 12;

    finalAngle = 0x400 - Atan2(coords[0] - car->x, coords[2] - car->z);

    if (car->shiftState == 0) {
        xValue = timer << 16;
        divisor = xValue >> 16;
        if (divisor <= 0) {
            divisor = 1;
        }

        headingDelta = GetAngleDelta(car->headingAngle, finalAngle);
        headingDelta = ((headingDelta * 5) << 2) / divisor;
        car->headingAngle += headingDelta;
    }
}

/*
 * Car motion-state handler for motionState == CAR_MOTION_TAKEOFF: the one-frame takeoff of a jump.
 * Turns the launch spin UpdateCarDriving seeded into clamped yaw, recomputes revs /
 * tacho / world velocity, then sets route+0x38 = 0x14 and route+0x98 = 2 to hand
 * the car to the airborne handler UpdateCarAirborne.
 */


void UpdateCarLaunch(PlayerCarRuntime *carArg, s32 unused) {
    register PlayerCarRuntime *car = carArg;
    register GameCarDrive *drive;
    register s32 s4val;
    s32 res;
    register s32 v0 asm("$2");
    register s32 first24 asm("$4");
    register s32 firstHeading asm("$5");
    u32 shiftRpmRange;

    (void)unused;

    first24 = car->bodyYaw;
    v0 = car->drive.spinRate;
    firstHeading = car->headingAngle;
    s4val = v0;
    if (v0 < 0) {
        s4val = -s4val;
    }

    res = GetAngleDistance(first24, firstHeading);
    drive = &car->drive;
    if (res >= 0x600) {
        car->speed = car->speed * 990 / 1000;
    }

    if (car->shiftState == 0) {
        s32 near;
        s32 phase;
        s32 volume;

        volume = 0x7F;
        near = res < 513;
        if (near) {
            volume = res / 8 + 0x40;
            asm("" : "=r"(res) : "0"(res));
            near = res < 513;
        }
        asm("" : "=r"(near) : "0"(near));
        if (near) {
            phase = res * 3 + 0x1800;
        } else {
            phase = 0x1E00;
        }
        SetIndexedEffectVoice(0, phase, volume);
    } else {
        SetIndexedEffectVoice(-1, 0, 0);
    }

    if (res < 0x80 && s4val < 0x800) {
        drive->launchEnergy -= (0x800 - s4val) * 4000 / 256;
    }
    if (car->speed < 0x190) {
        drive->launchEnergy -= (0x190 - car->speed) * 100;
    }

    if (drive->launchEnergy > 0) {
        s32 s2;
        u32 steerRange;

        drive->bodyLiftOffset += 10;
        if (drive->bodyLiftOffset >= 100) {
            drive->bodyLiftOffset = 100;
        }

        res = GetAngleDelta(car->bodyYaw, drive->targetHeading) * 98 / 100;
        s2 = res * (drive->steeringLoadAngle + 0x800);
        res = s2 / 2048;
        drive->spinRate += res * 16;

        steerRange = drive->steerPos + 127;
        if (steerRange < 255) {
            if (GetAngleDistance(car->bodyYaw, car->headingAngle) < 0x200) {
                drive->spinRate = drive->spinRate * 31 / 32;
                drive->spinRate =
                    GetAngleDelta(car->bodyYaw, car->headingAngle) + drive->spinRate;
            } else if (s4val < 0x800) {
                drive->spinRate = res / 2 + drive->spinRate;
            }
        }

        if (drive->spinRate > 0x3600) {
            drive->spinRate = 0x3600;
        }
        if (drive->spinRate < -0x3600) {
            drive->spinRate = -0x3600;
        }

        car->bodyYaw = drive->spinRate / 256 + car->bodyYaw;
        drive->launchEnergy -= 64;

        res = GetAngleDistance(car->bodyYaw, car->headingAngle);
        drive->launchEnergy -= res * res / 65536;
        drive->launchEnergy -= (0x3600 - s4val) / 64;

        {
            s32 a4 = car->speed;
            s32 half = drive->speedScale / 2;
            if (a4 < half) {
                drive->launchEnergy -= (half - a4) / 8;
            }
        }

        drive->launchEnergy -= drive->brakeInput * 4;
        drive->launchEnergy -= (0x100 - drive->acceleratorInput.value) * 4;
        car->speed -= drive->brakeInput * 10 / 256;
        car->speed -= (0x100 - drive->acceleratorInput.value) * 10 / 256;
    } else {
        drive->spinRate = drive->spinRate * 15 / 16;
        if (s4val < 0x1000) {
            GameCarSpec *spec;
            GameCarSpecAddress specAddress;
            s32 lo;
            s32 offset;

            {
                s32 gain = (100 - (drive->gear - 1) * 4) * 10000;
                drive->drivetrainTorque = gain * car->speed / 100;
            }
            drive->yawOffset = GetAngleDelta(car->headingAngle, car->bodyYaw);
            drive->launchHeading = car->headingAngle;
            car->headingAngle = car->bodyYaw;

            {
                register s32 t;
                register s32 sq asm("$3");

                t = drive->yawOffset;
                sq = t;
                if (t < 0) {
                    sq = -sq;
                }
                if (sq < 0x401) {
                    sq = sq * sq;
                } else {
                    sq = 0x800 - sq;
                    sq = sq * sq;
                }
                drive->launchSpeed = sq * car->speed / 0x100000;
            }

            drive->spinRate = 0;

            spec = g_CarSpec;
            lo = car->speed * 0xA0 / 1168 * 10000;
            specAddress.pointer = spec;
            specAddress.bytes += drive->gear << 2;
            lo /= specAddress.pointer->gearRatio[0];
            {
                EngineRpmAddress shiftTarget;

                asm volatile("" : : : "memory");
                offset = drive->gear;
                firstHeading = (u16)drive->engineRpm;
                offset <<= 2;
                asm volatile("" : :);
                RAW(drive->jumpTimer) = 0x14;
                RAW(drive->motionState) = CAR_MOTION_AIRBORNE;
                g_ShiftTargetRpm = lo;
                shiftTarget.value = &g_ShiftTargetRpm;
                drive->shiftRpmDelta = *shiftTarget.rpm - firstHeading;
                specAddress.pointer = spec;
                specAddress.bytes += offset;
            }
            {
                drive->engineLoad =
                    lo * specAddress.pointer->gearLoad[0] / 0x20000;
                if (drive->manual == 0) {
                    drive->engineLoad =
                        drive->engineLoad * 985 / 1000;
                }
            }

            shiftRpmRange = ((u16)drive->shiftRpmDelta + 99) & 0xFFFF;
            if (shiftRpmRange < 199) {
                g_ShiftSoundLevel = 1;
            } else {
                g_ShiftSoundLevel = 0;
            }
        }
    }

    drive->targetHeading = car->bodyYaw;
    SteerCarToTrackLine(car);

    res = GetAngleDistance(car->bodyYaw, car->headingAngle);
    if (res >= 0x401) {
        s32 factor;
        s32 a8;

        factor = (0x3600 - s4val) * 4;
        a8 = car->acceleration;
        factor = factor * a8;
        factor = factor * (res - 0x400);
        car->acceleration = a8 / 2 + factor / 14155776;
    } else {
        s32 a8 = car->acceleration;

        car->acceleration = (0x200 - res) * a8 / 512;
    }

    {
        s32 saved = car->headingAngle;
        AdvanceCarPosition(car);
        car->headingAngle = saved;
    }

    drive->accelPos = rsin(car->headingAngle) * car->speed / 256;
    drive->brakePos = rcos(car->headingAngle) * car->speed / 256;
}

/*
 * Car motion handler for motionState == CAR_MOTION_AIRBORNE (airborne / jump): decays velocity and
 * spin, advances the car (AdvanceCarPosition), and lands it when it returns to the
 * ground. The drive sub-block is the GameCarDrive view beginning at +0xBC.
 */
void UpdateCarAirborne(PlayerCarRuntime *car, s32 unused) {
    GameCarDrive *r = &car->drive;
    s32 sinF24;
    s32 cosF24;
    volatile s32 coords[3];

    (void)unused;
    s32 flag = g_ShiftSoundLevel;

    if (flag == 0) {
        s32 phase;
        s32 f11c = car->drive.yawOffset;
        if (f11c < 513) {
            phase = f11c * 3 + 6144;
        } else {
            phase = 0x1e00;
        }
        SetIndexedEffectVoice(0, phase, r->jumpTimer * 2 + 80);
    } else {
        SetIndexedEffectVoice(0, 0x1800, flag + 25);
    }

    {
        s32 rr = GetAngleDelta(car->bodyYaw, r->targetHeading);
        s32 base = car->bodyYaw;
        car->bodyYaw = rr / 5 + base;
        AdvanceCarPosition(car, base);
    }

    sinF24 = rsin(car->bodyYaw);
    cosF24 = rcos(car->bodyYaw);

    r->accelPos = rsin(car->headingAngle + r->yawOffset) * car->speed / 256;
    r->brakePos = rcos(car->headingAngle + r->yawOffset) * car->speed / 256;

    coords[0] = (cosF24 * r->accelPos - sinF24 * r->brakePos) / 4096;
    coords[2] = (sinF24 * r->accelPos + cosF24 * r->brakePos) / 4096;

    r->accelPos =
        rsin(r->launchHeading) * r->launchSpeed / 256 + sinF24 * coords[2] / 4096;
    r->brakePos =
        rcos(r->launchHeading) * r->launchSpeed / 256 + cosF24 * coords[2] / 4096;

    if (r->acceleratorLatch != 1 && r->brakeLatch != 1 && r->acceleratorInput.value < 128) {
        r->groundedFrames += 1;
    } else {
        r->groundedFrames = 0;
    }

    r->spinRate = r->spinRate * 31 / 32;
    r->launchSpeed = r->launchSpeed * 31 / 32;
    r->yawOffset = r->yawOffset * 31 / 32;

    r->bodyLiftOffset = r->bodyLiftOffset * 2 / 3;
    if (r->yawOffset >= 1537) {
        car->speed = car->speed * 4 / 5;
    }

    if (r->jumpTimer <= 0) {
        SetIndexedEffectVoice(-1, 0, 0);
        car->bodyYaw -= r->spinRate;
        g_ShiftSoundLevel = 0;
        r->shiftRpmDelta = 0;
        r->yawOffset = 0;
        r->launchSpeed = 0;
        r->motionState = CAR_MOTION_DRIVING;
        r->bodyLiftOffset = 0;
    }
}

void UpdateCarStandingStart(PlayerCarRuntime *car, s32 unused) {
    GameCarDrive *route = &car->drive;
    s32 sinA;
    s32 cosA;
    s32 base;
    s32 r;
    s32 coords[3];

    (void)unused;

    r = GetAngleDelta(car->bodyYaw, route->targetHeading);
    base = car->bodyYaw;
    car->bodyYaw = r / 5 + base;
    AdvanceCarPosition(car, base);

    sinA = rsin(car->bodyYaw);
    cosA = rcos(car->bodyYaw);

    route->accelPos = rsin(car->headingAngle) * car->speed / 256;
    route->brakePos = rcos(car->headingAngle) * car->speed / 256;

    coords[0] = (cosA * route->accelPos - sinA * route->brakePos) / 4096;
    coords[2] = (sinA * route->accelPos + cosA * route->brakePos) / 4096;
    route->accelPos = sinA * coords[2] / 16384;
    route->brakePos = cosA * coords[2] / 16384;

    SetIndexedEffectVoice(0, 0x1A80,
                          (0x60 - (g_StandingStartSpin & 0x1F) * 2) *
                              route->acceleratorInput.value / 256);

    car->speed = car->speed / 10;

    if (g_StandingStartSpin >= 11) {
        s32 f15c = route->acceleratorInput.value;
        s32 f134 = route->engineRpm;

        sinA = f15c + 32;
        g_StandingStartSpin -= route->brakeInput * 2;
        if (f134 < 2000) {
            sinA = f15c + 1032;
        }
        if (f15c < 127 && f134 >= 2001) {
            sinA += 127;
        }
        route->standingStartBounceY = (Random15() & 3) * sinA / 256;
        route->standingStartBounceX = (Random15() & 7) * sinA / 256;
        g_StandingStartSpin -= sinA;
        if (g_StandingStartSpin <= 0) {
            route->standingStartBounceY = 0;
            route->standingStartBounceX = 0;
            route->motionState = CAR_MOTION_DRIVING;
            SetIndexedEffectVoice(-1, 0, 0);
        }
    } else {
        SetIndexedEffectVoice(-1, 0, 0);
        route->motionState = CAR_MOTION_DRIVING;
        route->standingStartBounceY = 0;
        route->standingStartBounceX = 0;
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
        pa = &g_TrackPoints[i];
        pb = &g_TrackPoints[nxt];

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

    car->x = g_TrackPoints[i].x;
    car->z = g_TrackPoints[i].z;
    i = -1;
    asm volatile("" : "=r"(i) : "0"(i));
    return i;
}
