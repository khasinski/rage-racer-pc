#ifndef RAGE_PORT_TRACK_ASSET_IDENTITY_H
#define RAGE_PORT_TRACK_ASSET_IDENTITY_H

#include <stdint.h>

/* Keep the renderer tied to the track pack that is actually resident.  Some
 * scenes, notably the Grand Prix prologue, change g_CourseIndex after loading
 * their assets. */
void TrackAssetIdentitySet(int asset);
uint32_t TrackAssetIdentityResolve(uint32_t fallback);
uint64_t TrackAssetIdentityRevision(void);

#endif
