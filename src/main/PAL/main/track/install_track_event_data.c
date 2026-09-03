#include <stddef.h>

#include "game/asset.h"
#include "game/car.h"
#include "game/track_internal.h"

static void ClearTrackEventData(void) {
    g_TrackEventData = NULL;
    g_FlybySceneryData = NULL;
    g_RaceIntroCameraScript = NULL;
    g_RouteSceneryData = NULL;
    g_PathSceneryPosData = NULL;
    g_PathSceneryRotData = NULL;
}

static s32 TrackEventOffsetIsValid(s32 offset, size_t remaining) {
    if (offset < (s32)sizeof(TrackEventOffsets) ||
        offset % (s32)sizeof(s32) != 0 || (size_t)offset >= remaining) {
        return 0;
    }

    return 1;
}

static s32 TrackEventBlockFits(s32 start, s32 end, size_t headerSize,
                               size_t entrySize, size_t requiredEntries) {
    size_t blockSize;

    if (end <= start) return 0;
    blockSize = (size_t)(end - start);
    return blockSize >= headerSize &&
           requiredEntries <= (blockSize - headerSize) / entrySize;
}

static s32 SceneryMotionBlockIsValid(const TrackEventOffsets *offsets,
                                     s32 start, s32 end) {
    const SceneryMotionData *data =
        (const SceneryMotionData *)((const u8 *)offsets + start);
    size_t keyCount;
    s32 series;

    if (!TrackEventBlockFits(start, end,
                             offsetof(SceneryMotionData, keyframes),
                             sizeof(data->keyframes[0]), 2)) {
        return 0;
    }
    keyCount = ((size_t)(end - start) -
                offsetof(SceneryMotionData, keyframes)) /
               sizeof(data->keyframes[0]);
    for (series = 0; series < TRACK_SERIES_COUNT; series++) {
        s32 first = data->firstKeyframe[series][0];
        size_t i;

        if (first < 0 || (size_t)first + 1 >= keyCount) return 0;
        for (i = (size_t)first; i < keyCount; i++) {
            s16 duration = data->keyframes[i].duration;

            if (duration == SCENERY_MOTION_END) break;
            if (duration <= 0 || i + 1 >= keyCount) return 0;
        }
        if (i == (size_t)first || i == keyCount) return 0;
    }
    return 1;
}

static s32 RaceIntroCameraBlockIsValid(const TrackEventOffsets *offsets,
                                       s32 start, s32 end) {
    const RaceIntroCameraScript *script =
        (const RaceIntroCameraScript *)((const u8 *)offsets + start);
    size_t keyCount;
    s32 series;

    if (!TrackEventBlockFits(start, end,
                             offsetof(RaceIntroCameraScript, keys),
                             sizeof(script->keys[0]), 2)) {
        return 0;
    }
    keyCount = ((size_t)(end - start) -
                offsetof(RaceIntroCameraScript, keys)) /
               sizeof(script->keys[0]);
    for (series = 0; series < TRACK_SERIES_COUNT; series++) {
        s32 first = script->firstKeyIndex[series];
        size_t i;

        if (first < 0 || (size_t)first + 1 >= keyCount) return 0;
        for (i = (size_t)first; i < keyCount; i++) {
            if (script->keys[i].mode == 1) {
                if (script->keys[i].duration < 0) return 0;
                break;
            }
            if (script->keys[i].mode != 0 ||
                script->keys[i].duration <= 0) {
                return 0;
            }
            if (i + 1 >= keyCount) return 0;
        }
        if (i == keyCount) return 0;
    }
    return 1;
}

static s32 PathPositionSequenceIsValid(
    const PathSceneryPositionKey *keys, size_t keyCount, size_t first) {
    size_t i;

    for (i = first; i < keyCount; i++) {
        s16 span = keys[i].fields.span;

        if (span == -1) {
            size_t loop = keys[i].fields.loopIndex;
            return i > first && loop < i - first;
        }
        if (span < 0 || i + 1 >= keyCount) return 0;
    }
    return 0;
}

static s32 PathPositionBlockIsValid(const TrackEventOffsets *offsets,
                                    s32 start, s32 end) {
    const PathSceneryPositionData *data =
        (const PathSceneryPositionData *)((const u8 *)offsets + start);
    size_t keyCount;
    s32 series;

    if (!TrackEventBlockFits(start, end,
                             offsetof(PathSceneryPositionData, keys),
                             sizeof(data->keys[0]), 2)) {
        return 0;
    }
    keyCount = ((size_t)(end - start) -
                offsetof(PathSceneryPositionData, keys)) /
               sizeof(data->keys[0]);
    for (series = 0; series < TRACK_SERIES_COUNT; series++) {
        s32 first = data->firstKey[series];

        if (first < 0 || (size_t)first + 1 >= keyCount ||
            !PathPositionSequenceIsValid(data->keys, keyCount,
                                         (size_t)first)) {
            return 0;
        }
    }
    return 1;
}

static s32 PathRotationSequenceIsValid(
    const PathSceneryRotationKey *keys, size_t keyCount, size_t first) {
    size_t i;

    for (i = first; i < keyCount; i++) {
        s16 span = keys[i].fields.span;

        if (span == -1) {
            size_t loop = keys[i].fields.loopIndex;
            return i > first && loop < i - first;
        }
        if (span < 0 || i + 1 >= keyCount) return 0;
    }
    return 0;
}

static s32 PathRotationBlockIsValid(const TrackEventOffsets *offsets,
                                    s32 start, s32 end) {
    const PathSceneryRotationData *data =
        (const PathSceneryRotationData *)((const u8 *)offsets + start);
    size_t keyCount;
    s32 series;

    if (!TrackEventBlockFits(start, end,
                             offsetof(PathSceneryRotationData, keys),
                             sizeof(data->keys[0]), 2)) {
        return 0;
    }
    keyCount = ((size_t)(end - start) -
                offsetof(PathSceneryRotationData, keys)) /
               sizeof(data->keys[0]);
    for (series = 0; series < TRACK_SERIES_COUNT; series++) {
        s32 first = data->firstKey[series];

        if (first < 0 || (size_t)first + 1 >= keyCount ||
            !PathRotationSequenceIsValid(data->keys, keyCount,
                                         (size_t)first)) {
            return 0;
        }
    }
    return 1;
}

s32 IsValidTrackEventAsset(const TrackEventData *eventData, size_t size) {
    const TrackEventOffsets *offsets;
    size_t offsetsPosition = offsetof(TrackEventData, offsets);
    size_t remaining;
    s32 variableDataEnd =
        (s32)(offsetof(TrackEventData, eventSoundZones) - offsetsPosition);

    if (eventData == NULL || size < sizeof(*eventData)) {
        return 0;
    }

    offsets = &eventData->offsets;
    remaining = size - offsetsPosition;
    if (!TrackEventOffsetIsValid(offsets->flybyScenery, remaining) ||
        !TrackEventOffsetIsValid(offsets->routeScenery, remaining) ||
        !TrackEventOffsetIsValid(offsets->raceIntroCamera, remaining) ||
        !TrackEventOffsetIsValid(offsets->pathSceneryPosition, remaining) ||
        !TrackEventOffsetIsValid(offsets->pathSceneryRotation, remaining) ||
        offsets->pathSceneryRotation >= variableDataEnd) {
        return 0;
    }

    return SceneryMotionBlockIsValid(
               offsets, offsets->flybyScenery, offsets->routeScenery) &&
           SceneryMotionBlockIsValid(
               offsets, offsets->routeScenery, offsets->raceIntroCamera) &&
           RaceIntroCameraBlockIsValid(
               offsets, offsets->raceIntroCamera,
               offsets->pathSceneryPosition) &&
           PathPositionBlockIsValid(
               offsets, offsets->pathSceneryPosition,
               offsets->pathSceneryRotation) &&
           PathRotationBlockIsValid(
               offsets, offsets->pathSceneryRotation, variableDataEnd);
}

s32 InstallTrackEventData(TrackEventData *eventData, size_t size) {
    TrackEventOffsets *offsets;

    if (!IsValidTrackEventAsset(eventData, size)) {
        ClearTrackEventData();
        return 0;
    }
    offsets = &eventData->offsets;
    g_FlybySceneryData =
        (SceneryMotionData *)((u8 *)offsets + offsets->flybyScenery);
    g_RaceIntroCameraScript = (RaceIntroCameraScript *)(
        (u8 *)offsets + offsets->raceIntroCamera);
    g_RouteSceneryData =
        (SceneryMotionData *)((u8 *)offsets + offsets->routeScenery);
    g_PathSceneryPosData = (PathSceneryPositionData *)(
        (u8 *)offsets + offsets->pathSceneryPosition);
    g_PathSceneryRotData = (PathSceneryRotationData *)(
        (u8 *)offsets + offsets->pathSceneryRotation);
    g_TrackEventData = eventData;
    return 1;
}
