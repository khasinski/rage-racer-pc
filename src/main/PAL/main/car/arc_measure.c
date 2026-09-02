#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_state.h"
#include "game/track_internal.h"

static s32 MeasureArcRadius(s32 angle, s32 offsetX, s32 offsetZ) {
    return CarTrackFixed12ToInteger(
        rcos(angle) * offsetX + rsin(angle) * offsetZ);
}

/*
 * Where a car and the two track points around it sit on a curve's arc.
 *
 * Everything here is measured from the arc's centre: the offsets to it, the
 * angle each one stands at, and the radius each one is out by. The radius is
 * the offset projected onto its own angle, which is the offset's own length:
 * geometry alone never makes it meaningfully negative; fixed-point rounding
 * is shared with the other track projections.
 *
 * Two callers worked this out, thirty lines each, identical but for their
 * bracketing. They then diverge on what to do with the span between the two
 * points, which is why that part stays with them.
 */
void CarTrackMeasureArc(struct CarTrackWork *work, s32 arcIndex, s32 carX,
                        s32 carZ, const GameTrackPoint *point,
                        const GameTrackPoint *nextPoint) {
    const GameTrackArcCenter *arcCenter = &g_TrackArcCenters[arcIndex];
    s32 centerX = arcCenter->x;
    s32 centerZ = arcCenter->z;

    work->arcCenterX = centerX;
    work->arcCenterZ = centerZ;
    work->carToCenterX = carX - centerX;
    work->carToCenterZ = carZ - centerZ;
    work->pointToCenterX = point->x - centerX;
    work->pointToCenterZ = point->z - centerZ;
    work->nextPointToCenterX = nextPoint->x - centerX;
    work->nextPointToCenterZ = nextPoint->z - centerZ;

    work->sweptAngle = Atan2(work->carToCenterX, work->carToCenterZ) & 0xFFF;
    work->pointAngle = Atan2(work->pointToCenterX, work->pointToCenterZ) & 0xFFF;
    work->nextPointAngle =
        Atan2(work->nextPointToCenterX, work->nextPointToCenterZ) & 0xFFF;

    work->carRadius.value = MeasureArcRadius(
        work->sweptAngle, work->carToCenterX, work->carToCenterZ);
    work->pointRadius.value = MeasureArcRadius(
        work->pointAngle, work->pointToCenterX, work->pointToCenterZ);
    work->nextPointRadius.value = MeasureArcRadius(
        work->nextPointAngle, work->nextPointToCenterX,
        work->nextPointToCenterZ);
}
