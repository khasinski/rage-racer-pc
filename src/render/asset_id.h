#ifndef RAGE_ASSET_ID_H
#define RAGE_ASSET_ID_H

#include <stddef.h>
#include <stdint.h>

#include "render_world.h"

/* Build the stable, source-independent name exposed to mod manifests. The
 * returned name describes game content, never a RAGE.BIN archive entry. */
int AssetMaterialId(char *out, size_t capacity, uint32_t assetKey,
                        RageRenderAssetSet assetSet, uint32_t material);

/* Append an exact runtime material variant to a base material id. */
int AssetMaterialVariantId(char *out, size_t capacity,
                               uint32_t assetKey,
                               RageRenderAssetSet assetSet,
                               uint32_t material, uint8_t variant);

#endif
