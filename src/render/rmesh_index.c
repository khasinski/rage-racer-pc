#include "rmesh_index.h"
#include "asset_path.h"

#include <string.h>
#include <stdlib.h>

uint32_t RuntimeIndexVersion(const char *text, size_t size) {
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

static int Token(const char **cursor, const char *end,
                     const char **start, size_t *length) {
    const char *p = *cursor;
    while (p < end && (*p == ' ' || *p == '\t')) p++;
    *start = p;
    while (p < end && *p != ' ' && *p != '\t') p++;
    *length = (size_t)(p - *start);
    *cursor = p;
    return *length != 0;
}

static int ParseU32(const char *text, size_t length, uint32_t *out) {
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

static int EnvironmentRow(const char *line, const char *end, uint32_t maxDimension,
    uint32_t *key, RageRuntimeEnvironmentLocation *out) {
    const char *cursor = line, *fields[4];
    size_t lengths[4];
    if (memchr(line, '\0', (size_t)(end - line)) != NULL) return -1;
    while (cursor < end && (*cursor == ' ' || *cursor == '\t')) ++cursor;
    if (cursor == end || *cursor == '#') return 0;
    for (unsigned i = 0; i < 4; ++i)
        if (!Token(&cursor, end, &fields[i], &lengths[i])) return -1;
    while (cursor < end && (*cursor == ' ' || *cursor == '\t')) ++cursor;
    if (cursor != end || !ParseU32(fields[0], lengths[0], key) ||
        !ParseU32(fields[1], lengths[1], &out->width) ||
        !ParseU32(fields[2], lengths[2], &out->height) ||
        out->width == 0 || out->height == 0 ||
        out->width > maxDimension || out->height > maxDimension ||
        !AssetPathIsRelativeFile(fields[3], lengths[3])) return -1;
    out->path = fields[3];
    out->pathLength = lengths[3];
    return 1;
}

int EnvironmentIndexFind(const char *text, size_t size, uint32_t assetKey,
    uint32_t maxDimension, RageRuntimeEnvironmentLocation *out) {
    if (out == NULL) return 0;
    *out = (RageRuntimeEnvironmentLocation){0};
    if (text == NULL) return 0;
    const char *line = text, *end = text + size;
    while (line < end) {
        const char *lineEnd = line;
        while (lineEnd < end && *lineEnd != '\r' && *lineEnd != '\n') ++lineEnd;
        RageRuntimeEnvironmentLocation location = {0};
        uint32_t key;
        int parsed = EnvironmentRow(line, lineEnd, maxDimension, &key, &location);
        if (parsed < 0) return 0;
        if (parsed > 0 && key == assetKey) { *out = location; return 1; }
        line = lineEnd;
        if (line < end && *line++ == '\r' && line < end && *line == '\n') ++line;
    }
    return 0;
}

static int AssetSetName(RageRenderAssetSet set, const char **name,
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

typedef struct IndexIdentity {
    uint32_t key;
    unsigned set;
    size_t line;
} IndexIdentity;

static int CompareIdentity(const void *a, const void *b) {
    const IndexIdentity *left = a, *right = b;
    if (left->key != right->key) return left->key < right->key ? -1 : 1;
    if (left->set != right->set) return left->set < right->set ? -1 : 1;
    return left->line < right->line ? -1 : left->line != right->line;
}

int EnvironmentIndexValidate(const char *text, size_t size, uint32_t maxDimension,
                             size_t *errorLine) {
    IndexIdentity *keys = NULL;
    size_t count = 0, capacity = 0, lineNumber = 0, error = 0;
    if (errorLine != NULL) *errorLine = 0;
    if (text == NULL || maxDimension == 0) return 0;
    const char *line = text, *end = text + size;
    while (line < end) {
        const char *lineEnd = line;
        while (lineEnd < end && *lineEnd != '\r' && *lineEnd != '\n') ++lineEnd;
        ++lineNumber;
        RageRuntimeEnvironmentLocation location = {0};
        uint32_t key;
        int parsed = EnvironmentRow(line, lineEnd, maxDimension, &key, &location);
        if (parsed < 0) { error = lineNumber; goto fail; }
        if (parsed > 0) {
            if (count == capacity) {
                size_t next = capacity == 0 ? 16 : capacity * 2;
                if (next < capacity || next > SIZE_MAX / sizeof(*keys)) goto fail;
                void *grown = realloc(keys, next * sizeof(*keys));
                if (grown == NULL) goto fail;
                keys = grown;
                capacity = next;
            }
            keys[count++] = (IndexIdentity){key, 0, lineNumber};
        }
        line = lineEnd;
        if (line < end && *line++ == '\r' && line < end && *line == '\n') ++line;
    }
    if (count > 1) qsort(keys, count, sizeof(*keys), CompareIdentity);
    for (size_t i = 1; i < count; ++i)
        if (keys[i].key == keys[i - 1].key &&
            (error == 0 || keys[i].line < error)) error = keys[i].line;
    if (error != 0) goto fail;
    free(keys);
    return 1;
fail:
    free(keys);
    if (errorLine != NULL) *errorLine = error;
    return 0;
}

int RuntimeIndexValidate(const char *text, size_t size, size_t *errorLine) {
    static const RageRenderAssetSet sets[] = {
        RAGE_RENDER_ASSET_MODEL_BANK, RAGE_RENDER_ASSET_COURSE,
        RAGE_RENDER_ASSET_TERRAIN, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1,
        RAGE_RENDER_ASSET_TRACK_MODEL_BANK_2
    };
    IndexIdentity *identities = NULL;
    size_t count = 0, capacity = 0, lineNumber = 0, error = 0;
    if (errorLine != NULL) *errorLine = 0;
    if (RuntimeIndexVersion(text, size) != RAGE_RUNTIME_INDEX_VERSION) {
        if (errorLine != NULL) *errorLine = 1;
        return 0;
    }
    const char *line = text, *end = text + size;
    while (line < end) {
        const char *lineEnd = line;
        while (lineEnd < end && *lineEnd != '\n' && *lineEnd != '\r') ++lineEnd;
        ++lineNumber;
        const char *cursor = line;
        while (cursor < lineEnd && (*cursor == ' ' || *cursor == '\t')) ++cursor;
        if (memchr(line, '\0', (size_t)(lineEnd - line)) != NULL) goto invalid;
        if (cursor != lineEnd && *cursor != '#') {
            const char *key, *set, *mesh, *material;
            size_t keySize, setSize, meshSize, materialSize;
            uint32_t keyValue;
            if (!Token(&cursor, lineEnd, &key, &keySize) ||
                !Token(&cursor, lineEnd, &set, &setSize) ||
                !Token(&cursor, lineEnd, &mesh, &meshSize) ||
                !Token(&cursor, lineEnd, &material, &materialSize) ||
                !ParseU32(key, keySize, &keyValue) ||
                !AssetPathIsRelativeFile(mesh, meshSize) ||
                !AssetPathIsRelativeFile(material, materialSize)) goto invalid;
            while (cursor < lineEnd && (*cursor == ' ' || *cursor == '\t')) ++cursor;
            if (cursor != lineEnd) goto invalid;
            unsigned setIndex;
            for (setIndex = 0; setIndex < sizeof(sets) / sizeof(sets[0]); ++setIndex) {
                const char *name;
                size_t nameSize;
                if (AssetSetName(sets[setIndex], &name, &nameSize) &&
                    nameSize == setSize && memcmp(name, set, setSize) == 0) break;
            }
            if (setIndex == sizeof(sets) / sizeof(sets[0])) goto invalid;
            if (count == capacity) {
                size_t next = capacity == 0 ? 64 : capacity * 2;
                if (next < capacity || next > SIZE_MAX / sizeof(*identities)) goto done;
                void *grown = realloc(identities, next * sizeof(*identities));
                if (grown == NULL) goto done;
                identities = grown;
                capacity = next;
            }
            identities[count++] = (IndexIdentity){keyValue, setIndex, lineNumber};
        }
        line = lineEnd;
        if (line < end && *line++ == '\r' && line < end && *line == '\n') ++line;
    }
    if (count > 1) qsort(identities, count, sizeof(*identities), CompareIdentity);
    for (size_t i = 1; i < count; ++i) {
        if (identities[i].key == identities[i - 1].key &&
            identities[i].set == identities[i - 1].set &&
            (error == 0 || identities[i].line < error)) error = identities[i].line;
    }
    if (error != 0) goto done;
    free(identities);
    return 1;
invalid:
    error = lineNumber;
done:
    free(identities);
    if (errorLine != NULL) *errorLine = error;
    return 0;
}

int RuntimeIndexFind(const char *text, size_t size, uint32_t assetKey,
                         RageRenderAssetSet assetSet,
                         RageRuntimeAssetLocation *out) {
    const char *setName;
    size_t setLength;
    const char *line = text;
    const char *end;

    if (out == NULL) return 0;
    memset(out, 0, sizeof(*out));
    if (text == NULL || !AssetSetName(assetSet, &setName, &setLength)) return 0;
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
            Token(&cursor, lineEnd, &key, &keyLength) &&
            Token(&cursor, lineEnd, &set, &setTokenLength) &&
            Token(&cursor, lineEnd, &mesh, &meshLength) &&
            Token(&cursor, lineEnd, &material, &materialLength) &&
            ParseU32(key, keyLength, &keyValue) && keyValue == assetKey &&
            setTokenLength == setLength && memcmp(set, setName, setLength) == 0) {
            /* Reject a malformed matching record, rather than finding another
             * duplicate and silently changing which asset supplies the bytes. */
            while (cursor < lineEnd && (*cursor == ' ' || *cursor == '\t')) cursor++;
            if (cursor != lineEnd ||
                !AssetPathIsRelativeFile(mesh, meshLength) ||
                !AssetPathIsRelativeFile(material, materialLength)) return 0;
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
