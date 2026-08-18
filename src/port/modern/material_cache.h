#ifndef RAGE_MATERIAL_CACHE_H
#define RAGE_MATERIAL_CACHE_H

#include <stdint.h>

/* Decoded texture pages, kept until VRAM says otherwise.
 *
 * Measured on a running game: the pages holding textures are written when
 * assets load and then not touched again, while the frame buffer takes tens
 * of thousands of writes a minute. So entries are keyed by tpage and clut,
 * decoded on the first miss, and dropped only when a write lands on the VRAM
 * they came from. A revision counter would throw everything away several
 * times a frame and never pay for itself. */

/* Entries the cache holds. A race was measured to keep 271 page/palette
 * pairs in play at once; at 64 the cache thrashed, decoding 45k times in a
 * minute where 2k suffices. Each entry is a 256x256 RGBA page, so this
 * bounds the cache at 128 MB and in practice sits near 70. */
#define RAGE_MATERIAL_CACHE_SLOTS 512

/* Reads a VRAM region as RGBA8888, w*h*4 bytes into out. Returns non-zero on
 * success. Injected so the cache can be exercised without a GPU. */
typedef int (*RageVramReader)(int x, int y, int w, int h, void *out);

void RageMaterialCacheInit(RageVramReader reader);
void RageMaterialCacheShutdown(void);

/* The decoded 256x256 RGBA8 page for this tpage/clut pair, or NULL when the
 * VRAM read fails. Valid until the next invalidation that covers it. */
const uint8_t *RageMaterialCacheLookup(unsigned tpage, unsigned clut);

/* Drops entries whose source VRAM this write touched. Cheap enough to call
 * from the write observer. */
void RageMaterialCacheInvalidate(int x, int y, int w, int h);

/* Counters, for tests and diagnostics. */
typedef struct RageMaterialCacheStats {
    unsigned hits, misses, decodes, evictions, invalidations;
    unsigned live;
} RageMaterialCacheStats;

RageMaterialCacheStats RageMaterialCacheGetStats(void);

#endif
