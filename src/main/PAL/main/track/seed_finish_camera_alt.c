#include "game/track.h"
#include "game/race.h"
#include "game/car.h"

void SeedFinishCameraAlt(void *car) {
    u32 word0;
    u32 word1;
    u32 word2;
    Block16 *src;
    Block16 *dst;
    Block16 *end;
    GameCarRuntimeAddress source;
    GameCarRuntimeAddress destination;
    GameTrackPoint *track;
    TrackPointTableAddress pointAddress;
    TrackPointTableAddress trackAddress;
    GameTrackPoint *point;
    s32 index;
    s32 lastIndex;

    /* car is a car runtime block: the copy below moves 0x19C bytes of it into
     * g_CameraCar. Every one of the eleven g_CameraCar* split symbols lands
     * on one of its fields. Storing through
     * the source view is what lets the index reads below stay plain: both sides
     * carry the aggregate mark now, so 44a's exemption never fires. */
    source.runtime = car;
    
    destination.runtime = &g_CameraCar;
    dst = destination.blocks;
    src = source.blocks;
    end = src + sizeof(GameCarRuntime) / sizeof(*src);
    do {
        *dst = *src;
        src++;
        dst++;
    } while (src != end);

    word0 = src->w[0];
    word1 = src->w[1];
    word2 = src->w[2];
    dst->w[0] = word0;
    dst->w[1] = word1;
    dst->w[2] = word2;

    index = source.runtime->trackPointIndex;
    track = g_TrackPoints;
    pointAddress.pointOffset = (index * 3) << 3;
    trackAddress.pointPointer = track;
    pointAddress.value = pointAddress.pointOffset + trackAddress.value;
    point = pointAddress.pointPointer;
    g_CameraCar.x = point->x;

    index = source.runtime->trackPointIndex;
    pointAddress.pointOffset = (index * 3) << 3;
    trackAddress.pointPointer = track;
    pointAddress.value = pointAddress.pointOffset + trackAddress.value;
    point = pointAddress.pointPointer;
    g_CameraCar.z = point->z;

    index = source.runtime->trackPointIndex;
    pointAddress.pointOffset = (index * 3) << 3;
    trackAddress.pointPointer = track;
    pointAddress.value = pointAddress.pointOffset + trackAddress.value;
    point = pointAddress.pointPointer;
    word0 = point->y;
    index = g_GrandPrixSeries;
    g_CameraCar.speed = 0;
    g_CameraCar.y = word0 - 0x30;

    lastIndex = source.runtime->trackPointIndex;
    index <<= 11;
    pointAddress.pointOffset = (lastIndex * 3) << 3;
    trackAddress.pointPointer = track;
    pointAddress.value = pointAddress.pointOffset + trackAddress.value;
    point = pointAddress.pointPointer;
    index += 0xC00;
    index -= point->angle;
    g_CameraCar.headingAngle = index;
    g_CameraCarSeedYaw = index;
    g_CameraCar.bodyYaw = index;
}
