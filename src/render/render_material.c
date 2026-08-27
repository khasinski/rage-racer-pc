#include "render_material.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct RageMaterialToken {
    const char *text;
    size_t length;
} RageMaterialToken;

static int RageMaterialTokenEquals(RageMaterialToken token,
                                   const char *expected) {
    size_t length = strlen(expected);
    return token.length == length &&
           memcmp(token.text, expected, length) == 0;
}

static int RageMaterialNextToken(const char *line, size_t length,
                                 size_t *cursor, RageMaterialToken *token) {
    while (*cursor < length && line[*cursor] == ' ') (*cursor)++;
    if (*cursor == length) return 0;
    token->text = line + *cursor;
    while (*cursor < length && line[*cursor] != ' ') (*cursor)++;
    token->length = (size_t)(line + *cursor - token->text);
    return token->length != 0;
}

static int RageMaterialFloat(RageMaterialToken token, float *value) {
    char buffer[64];
    char *end;
    if (token.length == 0 || token.length >= sizeof(buffer)) return 0;
    memcpy(buffer, token.text, token.length);
    buffer[token.length] = '\0';
    *value = strtof(buffer, &end);
    return end == buffer + token.length && isfinite(*value);
}

static int RageMaterialProperties(const char *line, size_t length,
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
    if (!RageMaterialNextToken(line, length, &cursor, &token)) return 0;
    if (RageMaterialTokenEquals(token, "inherit"))
        material->shading = RAGE_RENDER_MATERIAL_SHADING_INHERIT;
    else if (RageMaterialTokenEquals(token, "lit"))
        material->shading = RAGE_RENDER_MATERIAL_SHADING_LIT;
    else if (RageMaterialTokenEquals(token, "unlit"))
        material->shading = RAGE_RENDER_MATERIAL_SHADING_UNLIT;
    else
        return 0;
    if (!RageMaterialNextToken(line, length, &cursor, &token)) return 0;
    if (RageMaterialTokenEquals(token, "auto"))
        material->alphaMode = RAGE_RENDER_MATERIAL_ALPHA_AUTO;
    else if (RageMaterialTokenEquals(token, "opaque"))
        material->alphaMode = RAGE_RENDER_MATERIAL_ALPHA_OPAQUE;
    else if (RageMaterialTokenEquals(token, "mask"))
        material->alphaMode = RAGE_RENDER_MATERIAL_ALPHA_MASK;
    else if (RageMaterialTokenEquals(token, "blend"))
        material->alphaMode = RAGE_RENDER_MATERIAL_ALPHA_BLEND;
    else
        return 0;
    for (value = 0; value < sizeof(values) / sizeof(values[0]); value++) {
        if (!RageMaterialNextToken(line, length, &cursor, &token) ||
            !RageMaterialFloat(token, values[value])) return 0;
    }
    while (cursor < length && line[cursor] == ' ') cursor++;
    return cursor == length && material->roughness >= 0.0f &&
           material->roughness <= 1.0f && material->metallic >= 0.0f &&
           material->metallic <= 1.0f;
}

void RageRenderMaterialDefault(RageRenderMaterial *material) {
    if (material == NULL) return;
    memset(material, 0, sizeof(*material));
    material->baseColorFactor[0] = 1.0f;
    material->baseColorFactor[1] = 1.0f;
    material->baseColorFactor[2] = 1.0f;
    material->baseColorFactor[3] = 1.0f;
    material->roughness = 1.0f;
}

int RageRenderMaterialParse(const void *bytes, size_t size,
                            uint32_t materialIndex, uint32_t variant,
                            RageRenderMaterial *material) {
    static const char v4[] = "# rage-rmat v4\n";
    static const char v5[] = "# rage-rmat v5\n";
    static const char v6[] = "# rage-rmat v6\n";
    const char *text = (const char *)bytes;
    size_t lineStart = 0, cursor;
    int version;
    if (bytes == NULL || material == NULL) return 0;
    RageRenderMaterialDefault(material);
    if (size >= sizeof(v6) - 1 && memcmp(text, v6, sizeof(v6) - 1) == 0)
        version = 6;
    else if (size >= sizeof(v5) - 1 &&
             memcmp(text, v5, sizeof(v5) - 1) == 0)
        version = 5;
    else if (size >= sizeof(v4) - 1 &&
             memcmp(text, v4, sizeof(v4) - 1) == 0)
        version = 4;
    else
        return 0;
    for (cursor = 0; cursor <= size; cursor++) {
        const char *line;
        size_t length, at = 0;
        uint32_t index = 0, pathIndex = 0;
        RageMaterialToken token;
        if (cursor != size && text[cursor] != '\n') continue;
        line = text + lineStart;
        length = cursor - lineStart;
        lineStart = cursor + 1;
        if (length == 0 || line[0] == '#') continue;
        while (at < length && line[at] >= '0' && line[at] <= '9') {
            index = index * 10u + (uint32_t)(line[at] - '0');
            at++;
        }
        if (at == 0 || at == length || line[at++] != ' ' ||
            index != materialIndex) continue;
        while (RageMaterialNextToken(line, length, &at, &token)) {
            if (RageMaterialTokenEquals(token, "|")) break;
            if (pathIndex++ == variant) {
                material->baseColorTexture.text = token.text;
                material->baseColorTexture.length = token.length;
            }
        }
        if (material->baseColorTexture.length == 0) return 0;
        if (version == 4) return at == length;
        if (!RageMaterialNextToken(line, length, &at, &token)) return 0;
        if (!RageMaterialTokenEquals(token, "-")) {
            material->paintMask.text = token.text;
            material->paintMask.length = token.length;
        }
        if (version == 5) {
            while (at < length && line[at] == ' ') at++;
            return at == length;
        }
        if (!RageMaterialNextToken(line, length, &at, &token) ||
            !RageMaterialTokenEquals(token, "|")) return 0;
        return RageMaterialProperties(line, length, at, material);
    }
    return 0;
}
