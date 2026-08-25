#ifndef RAGE_MOD_ASSETS_H
#define RAGE_MOD_ASSETS_H

#include <stddef.h>

/* Configured mod root shared by legacy archive overrides and native semantic
 * providers. It remains available even when the root has no raw/ tree. */
const char *RageModAssetsDirectory(void);

/* Load archive entry `index` from the override directory into `destination`.
 * Returns the byte count on success, with the low two bits cleared the way a
 * disc load reports one, or 0 when there is no override or it does not fit. */
int RageModAssetLoad(int index, void *destination, unsigned int originalSize);

/* Bytes usable from `at` to the end of the buffer it falls in, or 0 when the
 * pointer is not inside a buffer this port knows the extent of. */
size_t RagePortAssetRoomAt(const void *at);

/* Apply the override directory's edited PNGs to a loaded asset. */
void RageModPatchTextures(int index, void *data, size_t size);

#endif
