#include "game/car.h"
#include "game/race.h"
#include "game/render.h"
#include "game/scratchpad.h"
#include "game/track_internal.h"

/*
 * Where a car and the two track points around it sit on a curve's arc.
 *
 * Everything here is measured from the arc's centre: the offsets to it, the
 * angle each one stands at, and the radius each one is out by. The radius is
 * the offset projected onto its own angle, which is the offset's own length:
 * geometry alone never makes it negative. The bias before the shift is there
 * for the case where the fixed-point product overflows, and it rounds that
 * the way retail's division did. Nothing in the test sweep reaches it, and
 * removing it passes: it guards arithmetic, not shape.
 *
 * Two callers worked this out, thirty lines each, identical but for their
 * bracketing. They then diverge on what to do with the span between the two
 * points, which is why that part stays with them.
 */
void CarTrackMeasureArc(struct CarTrackScratch *spad, s32 arcIndex, s32 carX,
                        s32 carZ, const GameTrackPoint *point,
                        const GameTrackPoint *nextPoint) {
    const GameTrackArcCenter *arcCenter = &g_TrackArcCenters[arcIndex];
    s32 centerX = arcCenter->x;
    s32 centerZ = arcCenter->z;
    s32 radius;

    spad->arcCenterX = centerX;
    spad->arcCenterZ = centerZ;
    spad->carToCenterX = carX - centerX;
    spad->carToCenterZ = carZ - centerZ;
    spad->pointToCenterX = point->x - centerX;
    spad->pointToCenterZ = point->z - centerZ;
    spad->nextPointToCenterX = nextPoint->x - centerX;
    spad->nextPointToCenterZ = nextPoint->z - centerZ;

    spad->sweptAngle = Atan2(spad->carToCenterX, spad->carToCenterZ) & 0xFFF;
    spad->pointAngle = Atan2(spad->pointToCenterX, spad->pointToCenterZ) & 0xFFF;
    spad->nextPointAngle =
        Atan2(spad->nextPointToCenterX, spad->nextPointToCenterZ) & 0xFFF;

    radius = rcos(spad->sweptAngle) * spad->carToCenterX +
             rsin(spad->sweptAngle) * spad->carToCenterZ;
    if (radius < 0) radius += 0xFFF;
    spad->carRadius.value = radius >> 0xC;

    radius = rcos(spad->pointAngle) * spad->pointToCenterX +
             rsin(spad->pointAngle) * spad->pointToCenterZ;
    if (radius < 0) radius += 0xFFF;
    spad->pointRadius.value = radius >> 0xC;

    radius = rcos(spad->nextPointAngle) * spad->nextPointToCenterX +
             rsin(spad->nextPointAngle) * spad->nextPointToCenterZ;
    if (radius < 0) radius += 0xFFF;
    spad->nextPointRadius.value = radius >> 0xC;
}
