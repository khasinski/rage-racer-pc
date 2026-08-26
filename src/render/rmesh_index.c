#include "rmesh_index.h"

#include <string.h>

uint32_t RageRuntimeIndexVersion(const char *text, size_t size) {
    static const char prefix[] = "# rage-rmesh-index v";
    size_t cursor = sizeof(prefix) - 1;
    uint32_t version = 0;
    int digits = 0;
    if (text == NULL || size <= cursor ||
        memcmp(text, prefix, sizeof(prefix) - 1) != 0) return 0;
    while (cursor < size && text[cursor] >= '0' && text[cursor] <= '9') {
        uint32_t digit = (uint32_t)(text[cursor++] - '0');
        if (version > (UINT32_MAX - digit) / 10u) return 0;
        version = version * 10u + digit;
        digits = 1;
    }
    if (!digits || cursor >= size ||
        (text[cursor] != '\n' && text[cursor] != '\r')) return 0;
    return version;
}

static int RageToken(const char **cursor, const char *end,
                     const char **start, size_t *length) {
    const char *p = *cursor;
    while (p < end && (*p == ' ' || *p == '\t')) p++;
    *start = p;
    while (p < end && *p != ' ' && *p != '\t') p++;
    *length = (size_t)(p - *start);
    *cursor = p;
    return *length != 0;
}

static int RageParseU32(const char *text, size_t length, uint32_t *out) {
    uint32_t value = 0;
    size_t i;
    if (length == 0) return 0;
    for (i = 0; i < length; i++) {
        uint32_t digit;
        if (text[i] < '0' || text[i] > '9') return 0;
        digit = (uint32_t)(text[i] - '0');
        if (value > (UINT32_MAX - digit) / 10) return 0;
        value = value * 10 + digit;
    }
    *out = value;
    return 1;
}

static int RageAssetSetName(RageRenderAssetSet set, const char **name,
                            size_t *length) {
    switch (set) {
    case RAGE_RENDER_ASSET_MODEL_BANK: *name = "model"; *length = 5; return 1;
    case RAGE_RENDER_ASSET_COURSE: *name = "course"; *length = 6; return 1;
    case RAGE_RENDER_ASSET_TERRAIN: *name = "terrain"; *length = 7; return 1;
    case RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1:
        *name = "track-model-1"; *length = 13; return 1;
    case RAGE_RENDER_ASSET_TRACK_MODEL_BANK_2:
        *name = "track-model-2"; *length = 13; return 1;
    default: return 0;
    }
}

int RageRuntimeIndexFind(const char *text, size_t size, uint32_t assetKey,
                         RageRenderAssetSet assetSet,
                         RageRuntimeAssetLocation *out) {
    const char *setName;
    size_t setLength;
    const char *line = text;
    const char *end;

    if (text == 0 || out == 0 || !RageAssetSetName(assetSet, &setName, &setLength)) {
        return 0;
    }
    end = text + size;
    while (line < end) {
        const char *lineEnd = line;
        const char *cursor;
        const char *key, *set, *mesh, *material;
        size_t keyLength, setTokenLength, meshLength, materialLength;
        uint32_t keyValue;
        while (lineEnd < end && *lineEnd != '\n' && *lineEnd != '\r') lineEnd++;
        cursor = line;
        if (cursor < lineEnd && *cursor != '#' &&
            RageToken(&cursor, lineEnd, &key, &keyLength) &&
            RageToken(&cursor, lineEnd, &set, &setTokenLength) &&
            RageToken(&cursor, lineEnd, &mesh, &meshLength) &&
            RageToken(&cursor, lineEnd, &material, &materialLength) &&
            RageParseU32(key, keyLength, &keyValue) && keyValue == assetKey &&
            setTokenLength == setLength && memcmp(set, setName, setLength) == 0) {
            out->meshPath = mesh;
            out->meshPathLength = meshLength;
            out->materialPath = material;
            out->materialPathLength = materialLength;
            return 1;
        }
        line = lineEnd;
        while (line < end && (*line == '\n' || *line == '\r')) line++;
    }
    return 0;
}
