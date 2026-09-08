#ifndef RAGE_CLASSIC_MOTION_H
#define RAGE_CLASSIC_MOTION_H

#include "modern/scene_capture.h"

/* Presentation-only matching of the actual subdivided GP0 stream. Never
 * changes packets, GTE registers, ordering tables, or simulation state. */
void ClassicMotionReset(void);
void ClassicMotionPrepare(const RageSceneSnapshot *previous,
                          const RageClassicPacketSource *previousSources,
                          const RageSceneSnapshot *current,
                          const RageClassicPacketSource *currentSources);
/* Returns 1 only for an unambiguous, topology-compatible polygon match.
 * Coordinates already include the caller's drawing offset; apply deltas. */
int ClassicMotionCoordinates(int packetIndex, float fraction,
                             float x[4], float y[4]);
int ClassicMotionMatchCount(void);

#endif
