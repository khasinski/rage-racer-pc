#include "material_cache.h"

#include <stdlib.h>
#include <string.h>

#include "material_decode.h"

enum {
    RAGE_MATERIAL_SLOTS = RAGE_MATERIAL_CACHE_SLOTS,
    RAGE_PAGE_BYTES = RAGE_PAGE_TEXELS * RAGE_PAGE_TEXELS * 4,
    RAGE_CLUT_ENTRIES_MAX = 256
};

typedef struct RageRect {
    int x, y, w, h;
} RageRect;

typedef struct RageMaterialSlot {
    unsigned tpage, clut;
    unsigned lastUse;
    int used;
    RageRect pixels;  /* VRAM the texels came from */
    RageRect palette; /* VRAM the CLUT row came from */
    uint8_t *texels;
} RageMaterialSlot;

static RageMaterialSlot s_slots[RAGE_MATERIAL_SLOTS];
static RageVramReader s_reader;
static RageMaterialCacheStats s_stats;
static unsigned s_clock;

static int RageRectsOverlap(RageRect a, RageRect b) {
    return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h &&
           b.y < a.y + a.h;
}

void RageMaterialCacheInit(RageVramReader reader) {
    RageMaterialCacheShutdown();
    s_reader = reader;
}

void RageMaterialCacheShutdown(void) {
    int i;
    for (i = 0; i < RAGE_MATERIAL_SLOTS; i++) {
        free(s_slots[i].texels);
        s_slots[i].texels = NULL;
        s_slots[i].used = 0;
    }
    memset(&s_stats, 0, sizeof(s_stats));
    s_clock = 0;
    s_reader = NULL;
}

static RageMaterialSlot *RageFindSlot(unsigned tpage, unsigned clut) {
    int i;
    for (i = 0; i < RAGE_MATERIAL_SLOTS; i++) {
        if (s_slots[i].used && s_slots[i].tpage == tpage &&
            s_slots[i].clut == clut) {
            return &s_slots[i];
        }
    }
    return NULL;
}

/* Free slot, else the one used longest ago. */
static RageMaterialSlot *RageClaimSlot(void) {
    int i, oldest = 0;
    for (i = 0; i < RAGE_MATERIAL_SLOTS; i++) {
        if (!s_slots[i].used) return &s_slots[i];
        if (s_slots[i].lastUse < s_slots[oldest].lastUse) oldest = i;
    }
    s_stats.evictions++;
    return &s_slots[oldest];
}

const uint8_t *RageMaterialCacheLookup(unsigned tpage, unsigned clut) {
    RageTexturePage page = RageTexturePageFromTpage(tpage);
    RageMaterialSlot *slot;
    uint8_t *pixels = NULL;
    uint8_t *palette = NULL;
    int clutX, clutY, clutEntries, ok = 0;

    if (s_reader == NULL) return NULL;
    /* A 16bpp page carries colour directly, so its clut field means nothing
     * and folding it into the key would store the same texels many times. */
    if (page.mode >= RAGE_TEXTURE_MODE_16BIT) clut = 0;

    slot = RageFindSlot(tpage, clut);
    if (slot != NULL) {
        s_stats.hits++;
        slot->lastUse = ++s_clock;
        return slot->texels;
    }
    s_stats.misses++;

    RageClutOrigin(clut, &clutX, &clutY);
    clutEntries = page.mode == RAGE_TEXTURE_MODE_8BIT ? 256 : 16;
    pixels = malloc((size_t)page.words * RAGE_PAGE_TEXELS * 4);
    palette = malloc((size_t)RAGE_CLUT_ENTRIES_MAX * 4);
    if (pixels != NULL && palette != NULL) {
        ok = s_reader(page.x, page.y, page.words, RAGE_PAGE_TEXELS, pixels);
        if (ok && page.mode < RAGE_TEXTURE_MODE_16BIT) {
            ok = s_reader(clutX, clutY, clutEntries, 1, palette);
        }
    }
    if (!ok) {
        free(pixels);
        free(palette);
        return NULL;
    }

    slot = RageClaimSlot();
    if (slot->texels == NULL) slot->texels = malloc(RAGE_PAGE_BYTES);
    if (slot->texels == NULL) {
        free(pixels);
        free(palette);
        return NULL;
    }
    RageDecodeTexturePageRegions(pixels, page.words * 4,
                                 page.mode < RAGE_TEXTURE_MODE_16BIT ? palette
                                                                     : NULL,
                                 page.mode, slot->texels);
    slot->tpage = tpage;
    slot->clut = clut;
    slot->used = 1;
    slot->lastUse = ++s_clock;
    slot->pixels = (RageRect){page.x, page.y, page.words, RAGE_PAGE_TEXELS};
    slot->palette = (RageRect){clutX, clutY, clutEntries, 1};
    s_stats.decodes++;
    free(pixels);
    free(palette);
    return slot->texels;
}

void RageMaterialCacheInvalidate(int x, int y, int w, int h) {
    RageRect write = {x, y, w, h};
    int i;
    if (w <= 0 || h <= 0) return;
    for (i = 0; i < RAGE_MATERIAL_SLOTS; i++) {
        if (!s_slots[i].used) continue;
        if (!RageRectsOverlap(write, s_slots[i].pixels) &&
            !RageRectsOverlap(write, s_slots[i].palette)) {
            continue;
        }
        s_slots[i].used = 0;
        s_stats.invalidations++;
    }
}

RageMaterialCacheStats RageMaterialCacheGetStats(void) {
    RageMaterialCacheStats stats = s_stats;
    int i;
    stats.live = 0;
    for (i = 0; i < RAGE_MATERIAL_SLOTS; i++)
        if (s_slots[i].used) stats.live++;
    return stats;
}
