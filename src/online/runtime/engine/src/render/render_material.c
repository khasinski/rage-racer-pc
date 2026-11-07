#include "render_material.h"
#include "asset_path.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct RageMaterialToken {
    const char *text;
    size_t length;
} RageMaterialToken;

static int MaterialTokenEquals(RageMaterialToken token,
                                   const char *expected) {
    size_t length = strlen(expected);
    return token.length == length &&
           memcmp(token.text, expected, length) == 0;
}

static int MaterialNextToken(const char *line, size_t length,
                                 size_t *cursor, RageMaterialToken *token) {
    while (*cursor < length && line[*cursor] == ' ') (*cursor)++;
    if (*cursor == length) return 0;
    token->text = line + *cursor;
    while (*cursor < length && line[*cursor] != ' ') (*cursor)++;
    token->length = (size_t)(line + *cursor - token->text);
    return token->length != 0;
}

static int MaterialFloat(RageMaterialToken token, float *value) {
    char buffer[64];
    char *end;
    if (token.length == 0 || token.length >= sizeof(buffer)) return 0;
    memcpy(buffer, token.text, token.length);
    buffer[token.length] = '\0';
    *value = strtof(buffer, &end);
    return end == buffer + token.length && isfinite(*value);
}

static int MaterialIndex(const char *line, size_t length, size_t *cursor,
                         uint32_t *index) {
    uint32_t value = 0;
    size_t start = *cursor;
    while (*cursor < length && line[*cursor] >= '0' &&
           line[*cursor] <= '9') {
        uint32_t digit = (uint32_t)(line[*cursor] - '0');
        if (value > (UINT32_MAX - digit) / 10u) return 0;
        value = value * 10u + digit;
        (*cursor)++;
    }
    if (*cursor == start) return 0;
    *index = value;
    return 1;
}

static int MaterialProperties(const char *line, size_t length,
                                  size_t cursor,
                                  RageRenderMaterial *material) {
    RageMaterialToken token;
    float *values[] = {
        &material->roughness, &material->metallic,
        &material->baseColorFactor[0], &material->baseColorFactor[1],
        &material->baseColorFactor[2], &material->baseColorFactor[3],
        &material->emissiveFactor[0], &material->emissiveFactor[1],
        &material->emissiveFactor[2],
    };
    size_t value;
    if (!MaterialNextToken(line, length, &cursor, &token)) return 0;
    if (MaterialTokenEquals(token, "inherit"))
        material->shading = RAGE_RENDER_MATERIAL_SHADING_INHERIT;
    else if (MaterialTokenEquals(token, "lit"))
        material->shading = RAGE_RENDER_MATERIAL_SHADING_LIT;
    else if (MaterialTokenEquals(token, "unlit"))
        material->shading = RAGE_RENDER_MATERIAL_SHADING_UNLIT;
    else
        return 0;
    if (!MaterialNextToken(line, length, &cursor, &token)) return 0;
    if (MaterialTokenEquals(token, "auto"))
        material->alphaMode = RAGE_RENDER_MATERIAL_ALPHA_AUTO;
    else if (MaterialTokenEquals(token, "opaque"))
        material->alphaMode = RAGE_RENDER_MATERIAL_ALPHA_OPAQUE;
    else if (MaterialTokenEquals(token, "mask"))
        material->alphaMode = RAGE_RENDER_MATERIAL_ALPHA_MASK;
    else if (MaterialTokenEquals(token, "blend"))
        material->alphaMode = RAGE_RENDER_MATERIAL_ALPHA_BLEND;
    else
        return 0;
    for (value = 0; value < sizeof(values) / sizeof(values[0]); value++) {
        if (!MaterialNextToken(line, length, &cursor, &token) ||
            !MaterialFloat(token, values[value])) return 0;
    }
    while (cursor < length && line[cursor] == ' ') cursor++;
    if (cursor != length) return 0;
    for (value = 0; value < sizeof(values) / sizeof(values[0]); value++)
        if (*values[value] < 0.0f || *values[value] > 1.0f) return 0;
    return 1;
}

int RenderMaterialParseProperties(const char *text, size_t size,
                                  RageRenderMaterial *material) {
    RageRenderMaterial parsed;

    if (text == NULL || material == NULL) return 0;
    parsed = *material;
    if (!MaterialProperties(text, size, 0, &parsed)) return 0;
    *material = parsed;
    return 1;
}

void RenderMaterialDefault(RageRenderMaterial *material) {
    if (material == NULL) return;
    memset(material, 0, sizeof(*material));
    material->baseColorFactor[0] = 1.0f;
    material->baseColorFactor[1] = 1.0f;
    material->baseColorFactor[2] = 1.0f;
    material->baseColorFactor[3] = 1.0f;
    material->roughness = 1.0f;
}

/* The catalog owns one immutable text snapshot and a sorted record table.
 * Variant path spans stay in that snapshot; numeric properties are decoded
 * once. Neither lookup nor another catalog can invalidate returned paths. */
struct RageMaterialCatalogEntry {
    uint32_t index;
    RageRenderMaterial definition;
    RageRenderMaterialPath variants;
};

static int MaterialVersion(const char *text, size_t size) {
    static const char prefix[] = "# rage-rmat v";
    const size_t n = sizeof(prefix) - 1;
    if (!text || size < n + 2 || memcmp(text, prefix, n) ||
        text[n + 1] != '\n' || text[n] < '4' || text[n] > '6') return 0;
    return text[n] - '0';
}

static int MaterialRecord(const char *line, size_t length, int version,
                           RageMaterialCatalogEntry *entry) {
    size_t at = 0, pathStart;
    RageMaterialToken token;
    int separator = 0;
    RenderMaterialDefault(&entry->definition);
    if (!MaterialIndex(line, length, &at, &entry->index) ||
        at == length || line[at++] != ' ') return 0;
    pathStart = at;
    entry->variants.text = line + at;
    entry->variants.length = 0;
    while (MaterialNextToken(line, length, &at, &token)) {
        if (MaterialTokenEquals(token, "|")) { separator = 1; break; }
        if (!AssetPathIsRelativeFile(token.text, token.length)) return 0;
        entry->variants.length = at - pathStart;
    }
    if (!entry->variants.length) return 0;
    if (version == 4) return !separator;
    if (!separator || !MaterialNextToken(line, length, &at, &token)) return 0;
    if (!MaterialTokenEquals(token, "-")) {
        if (!AssetPathIsRelativeFile(token.text, token.length)) return 0;
        entry->definition.paintMask = (RageRenderMaterialPath){token.text, token.length};
    }
    if (version == 5) {
        while (at < length && line[at] == ' ') ++at;
        return at == length;
    }
    if (!MaterialNextToken(line, length, &at, &token) ||
        !MaterialTokenEquals(token, "|")) return 0;
    return MaterialProperties(line, length, at, &entry->definition);
}

static int MaterialEntryCompare(const void *left, const void *right) {
    const RageMaterialCatalogEntry *a = left, *b = right;
    return (a->index > b->index) - (a->index < b->index);
}

void RenderMaterialCatalogRelease(RageMaterialCatalog *catalog) {
    if (!catalog) return;
    free(catalog->entries);
    free(catalog->bytes);
    memset(catalog, 0, sizeof(*catalog));
}

int RenderMaterialCatalogOpen(RageMaterialCatalog *catalog,
                              const void *bytes, size_t size) {
    RageMaterialCatalog next = {0};
    size_t capacity = 0, start = 0;
    int version = MaterialVersion(bytes, size);
    if (!catalog || !version) return 0;
    next.bytes = malloc(size);
    if (!next.bytes) return 0;
    memcpy(next.bytes, bytes, size);
    for (size_t end = 0; end <= size; ++end) {
        RageMaterialCatalogEntry entry;
        if (end != size && next.bytes[end] != '\n') continue;
        const char *line = next.bytes + start;
        size_t length = end - start;
        start = end + 1;
        if (!length || line[0] == '#') continue;
        if (!MaterialRecord(line, length, version, &entry)) goto failed;
        if (next.count == capacity) {
            size_t grown = capacity ? capacity * 2 : 16;
            if (grown < capacity || grown > SIZE_MAX / sizeof(*next.entries)) goto failed;
            void *entries = realloc(next.entries, grown * sizeof(*next.entries));
            if (!entries) goto failed;
            next.entries = entries;
            capacity = grown;
        }
        next.entries[next.count++] = entry;
    }
    if (next.count > 1) {
        qsort(next.entries, next.count, sizeof(*next.entries), MaterialEntryCompare);
        for (size_t i = 1; i < next.count; ++i)
            if (next.entries[i - 1].index == next.entries[i].index) goto failed;
    }
    RenderMaterialCatalogRelease(catalog);
    *catalog = next;
    return 1;
failed:
    RenderMaterialCatalogRelease(&next);
    return 0;
}

int RenderMaterialCatalogFind(const RageMaterialCatalog *catalog,
                              uint32_t materialIndex, uint32_t variant,
                              RageRenderMaterial *material) {
    size_t low = 0, high;
    if (!catalog || !material) return 0;
    high = catalog->count;
    while (low < high) {
        size_t mid = low + (high - low) / 2;
        if (catalog->entries[mid].index < materialIndex) low = mid + 1;
        else high = mid;
    }
    if (low == catalog->count || catalog->entries[low].index != materialIndex) return 0;
    const RageMaterialCatalogEntry *entry = &catalog->entries[low];
    size_t at = 0;
    RageMaterialToken token;
    while (MaterialNextToken(entry->variants.text, entry->variants.length, &at, &token)) {
        if (variant) { --variant; continue; }
        RageRenderMaterial result = entry->definition;
        result.baseColorTexture = (RageRenderMaterialPath){token.text, token.length};
        *material = result;
        return 1;
    }
    return 0;
}

int RenderMaterialParse(const void *bytes, size_t size,
                         uint32_t materialIndex, uint32_t variant,
                         RageRenderMaterial *material) {
    RageMaterialCatalog catalog = {0};
    RageRenderMaterial parsed;
    if (!material || !RenderMaterialCatalogOpen(&catalog, bytes, size)) return 0;
    int found = RenderMaterialCatalogFind(&catalog, materialIndex, variant, &parsed);
    if (found) {
        parsed.baseColorTexture.text = (const char *)bytes +
            (parsed.baseColorTexture.text - catalog.bytes);
        if (parsed.paintMask.text)
            parsed.paintMask.text = (const char *)bytes +
                (parsed.paintMask.text - catalog.bytes);
        *material = parsed;
    }
    RenderMaterialCatalogRelease(&catalog);
    return found;
}
