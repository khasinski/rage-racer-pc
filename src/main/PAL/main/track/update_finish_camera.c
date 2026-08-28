#include "common.h"
#include "game/car.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/scratchpad.h"
#include "game/track.h"
#include "game/vector.h"
#include "psyq/gte.h"


/*
 * Camera track-follower: advances a look-ahead track point, aims the eye object
 * g_CameraCar toward the sampled centre-line point
 * (InterpolateTrackPoint + atan2), nudges its position, then seeds the scratchpad view
 * state (view[2..4]=eye XYZ, view[6]=pitch, view[7]=yaw, view[8]=roll) from the
 * eye object and submits the render object (DrawPlayerCarModel). markerClamp is
 * the complete zeroed track-limit record passed to UpdateCarTrackState.
 */
void UpdateFinishCamera(GameRenderObject *obj) {
    ScratchLegacyViewWords legacyView;
    s32 *view;
    s32 delta[3];
    s32 coords[3];
    CarTrackLimits markerClamp;
    s32 index;
    s32 rem;
    s32 offset;
    s32 angle;
    s32 value;
    s32 zValue;
    GameCarRuntimeAddress cameraAddress;
    ScratchBlockAddress viewAddress;

    LoadScratchLegacyView(&legacyView);
    view = legacyView.words;
    offset = g_CameraCarTrackPoint;
    if (obj->facingBackwards != 0) {
        index = offset + 2;
    } else {
        index = offset - 2;
    }
    rem = index;
    if (index < 0) {
        rem = index + g_TrackPointCount;
    }
    index = rem % g_TrackPointCount;

    InterpolateTrackPoint(index, coords, g_CameraCar.segmentFraction);
    angle = 0x400 - Atan2(coords[0] - g_CameraCar.x, coords[2] - g_CameraCarZ);

    g_CameraCarHeading += GetAngleDelta(g_CameraCarHeading, angle);

    value = rsin(g_CameraCarHeading) * g_CameraCarSpeed;
    if (value < 0) {
        value += 0xFF;
    }
    g_CameraCarStepX = value >> 8;

    zValue = rcos(g_CameraCarHeading) * g_CameraCarSpeed;
    if (zValue < 0) {
        zValue += 0xFF;
    }
    g_CameraCarStepZ = zValue >> 8;

    g_CameraCar.x = g_CameraCarStepX / 256 + g_CameraCar.x;
    g_CameraCarZ = g_CameraCarStepZ / 256 + g_CameraCarZ;

    AccumulateLapProgress(&g_CameraCar);
    markerClamp.rightInset = 0;
    markerClamp.leftInset = 0;
    markerClamp.rightKnockbackMode = 0;
    markerClamp.leftKnockbackMode = 0;
    UpdateCarTrackState(&g_CameraCar, g_CameraCarTrackPoint, &markerClamp);

    cameraAddress.runtime = &g_CameraCar;
    viewAddress.words = &view[2];
    viewAddress.blocks[0] = cameraAddress.blocks[0];
    view[3] -= 64;

    delta[0] = obj->x - view[2];
    delta[1] = obj->y - view[3];
    delta[2] = obj->z - view[4];

    view[7] = 0x400 - Atan2(delta[0], delta[2]);
    value = DistanceXZ(delta[0], delta[2]);
    view[6] = 0x400 - Atan2(delta[1], value >> 6);
    view[8] = 0;

    StoreScratchLegacyView(&legacyView);
    SetCameraRotMatrix();
    SelectModelBank(0);
    DrawPlayerCarModel(obj);
}
