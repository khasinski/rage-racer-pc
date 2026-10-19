#ifndef RAGE_MOD_ASSETS_H
#define RAGE_MOD_ASSETS_H

#include <stddef.h>
struct RageModManifest;

/* Configured mod root shared by legacy archive overrides and native semantic
 * providers. It remains available even when the root has no raw/ tree. */
const char *ModAssetsDirectory(void);
/* Disc identity is supplied before any assets are installed. Hash only when
 * a pointer exists; normal unmodded startup does not reread the archive. */
int ModAssetsNeedsSource(void);
void ModAssetsSetSource(const char *region,const char *fingerprint);
const char *ModAssetsCarProfile(void);
/* Shared validated manifest, or NULL for legacy-only/disabled mods. Borrowed
 * session-lifetime view; the configured mod is loaded once, without GPU/SDL. */
const struct RageModManifest *ModAssetsManifest(void);
/* Invalidate all borrowed directory/manifest views after consumers have been
 * retired. Idempotent. Next access rereads configuration and manifest. This
 * retains mounted-disc identity, does not unload already-installed game data;
 * not a mid-race hot-reload API. */
void ModAssetsShutdown(void);
unsigned ModAssetsGeneration(void);

/* Load archive entry `index` from the override directory into `destination`.
 * Returns the byte count on success, with the low two bits cleared the way a
 * disc load reports one, or 0 when there is no override or it does not fit. */
/* Successful raw load in this asset generation; no filesystem query. */
int ModAssetWasReplaced(unsigned index);
void ModAssetSetGeometryChanged(unsigned index,int changed);
int ModAssetGeometryChanged(unsigned index);
int ModAssetExists(int index);
void ModAssetCourseLabel(int extraCourseIndex, char *nameOut,
                         size_t nameCapacity, char *lengthOut,
                         size_t lengthCapacity);
int ModAssetLoad(int index, void *destination, unsigned int originalSize);

/* Bytes usable from `at` to the end of the buffer it falls in, or 0 when the
 * pointer is not inside a buffer this port knows the extent of. */
size_t PortAssetRoomAt(const void *at);

/* Apply the override directory's edited PNGs to a loaded asset. */
void ModPatchTextures(int index, void *data, size_t size);

#endif
