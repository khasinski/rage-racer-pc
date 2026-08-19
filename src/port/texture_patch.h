#ifndef RAGE_TEXTURE_PATCH_H
#define RAGE_TEXTURE_PATCH_H

#include <stddef.h>

/* Apply every edited PNG belonging to `assetIndex` to an asset already in
 * memory. Returns how many textures were changed, or -1 when the mod directory
 * carries no texture index. Problems are reported on stderr naming the file. */
int RageTexturePatchAsset(const char *directory, int assetIndex,
                          unsigned char *data, size_t size);

#endif
