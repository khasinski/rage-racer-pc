#include "asset_id.h"

#include <stdio.h>

static int TrackName(char *out, size_t capacity, uint32_t assetKey) {
    static const char *const courses[] = {"big", "mid", "hi", "oval"};
    uint32_t offset, course, classNumber;
    int written;

    /* Track Render World keys are the .2ND pack for BIG1 through OVAL6. */
    if (assetKey < 0x58u || assetKey > 0x86u ||
        ((assetKey - 0x58u) & 1u) != 0) return 0;
    offset = (assetKey - 0x58u) / 2u;
    course = offset % 4u;
    classNumber = offset / 4u + 1u;
    written = snprintf(out, capacity, "%s%u", courses[course], classNumber);
    return written >= 0 && (size_t)written < capacity;
}

static const char *AssetSetMaterialName(RageRenderAssetSet assetSet) {
    switch (assetSet) {
    case RAGE_RENDER_ASSET_COURSE: return "course";
    case RAGE_RENDER_ASSET_TERRAIN: return "terrain";
    case RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1: return "model-bank-1";
    case RAGE_RENDER_ASSET_TRACK_MODEL_BANK_2: return "model-bank-2";
    default: return NULL;
    }
}

static int AssetIdWasWritten(char *out, size_t capacity, int written) {
    if (written < 0 || (size_t)written >= capacity) {
        out[0] = '\0';
        return 0;
    }
    return 1;
}

int AssetMaterialId(char *out, size_t capacity, uint32_t assetKey,
                        RageRenderAssetSet assetSet, uint32_t material) {
    const char *setName;
    char track[16];
    int written;

    if (out == NULL || capacity == 0) return 0;
    out[0] = '\0';
    if (assetSet == RAGE_RENDER_ASSET_MODEL_BANK) {
        uint32_t car;
        if (assetKey < 10u || assetKey > 72u || ((assetKey - 10u) & 1u) != 0)
            return 0;
        car = (assetKey - 10u) / 2u;
        written = snprintf(out, capacity, "car.%u.material.%u", car,
                           material);
        return AssetIdWasWritten(out, capacity, written);
    }
    setName = AssetSetMaterialName(assetSet);
    if (setName == NULL || !TrackName(track, sizeof(track), assetKey))
        return 0;
    written = snprintf(out, capacity, "track.%s.%s.material.%u", track,
                       setName, material);
    return AssetIdWasWritten(out, capacity, written);
}

int AssetMaterialVariantId(char *out, size_t capacity,
                           uint32_t assetKey,
                           RageRenderAssetSet assetSet,
                           uint32_t material, uint8_t variant) {
    char base[128];
    int written;
    if (out == NULL || capacity == 0) return 0;
    out[0] = '\0';
    if (!AssetMaterialId(base, sizeof(base), assetKey, assetSet, material))
        return 0;
    written = snprintf(out, capacity, "%s.variant.%u", base,
                       (unsigned)variant);
    return AssetIdWasWritten(out, capacity, written);
}
