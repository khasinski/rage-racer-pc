#include "mod_manifest.h"
#include "mod_selection.h"

#include <ctype.h>
#include <limits.h>
#include <string.h>

#include "render_material.h"
#include "asset_path.h"

typedef enum RageModSection {
    RAGE_MOD_SECTION_NONE,
    RAGE_MOD_SECTION_MOD,
    RAGE_MOD_SECTION_TEXTURES,
    RAGE_MOD_SECTION_MATERIALS,
    RAGE_MOD_SECTION_MESHES,
} RageModSection;

static char *ManifestTrim(char *text) {
    char *end;
    while (isspace((unsigned char)*text)) text++;
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) end--;
    *end = '\0';
    return text;
}

static int ManifestString(const char **cursor, char *out,
                              size_t capacity) {
    const char *at = *cursor;
    size_t used = 0;
    if (*at != '"' || capacity == 0) return 0;
    at++;
    while (*at != '\0' && *at != '"') {
        char value = *at++;
        if (value == '\\') {
            value = *at++;
            if (value != '\\' && value != '"') return 0;
        }
        if (used + 1 >= capacity) return 0;
        out[used++] = value;
    }
    if (*at != '"') return 0;
    out[used] = '\0';
    *cursor = at + 1;
    return used != 0;
}

static int ManifestLineEnd(const char *cursor) {
    while (isspace((unsigned char)*cursor)) cursor++;
    return *cursor == '\0' || *cursor == '#';
}

static int ManifestRelativePath(const char *path) {
    return AssetPathIsRelativeFile(path, strlen(path));
}

static int ManifestSemanticId(const char *key) {
    const unsigned char *at = (const unsigned char *)key;
    if (*at == '\0') return 0;
    while (*at != '\0') {
        if (!(*at >= 'a' && *at <= 'z') && !(*at >= '0' && *at <= '9') &&
            *at != '.' && *at != '-')
            return 0;
        at++;
    }
    return 1;
}

static int ManifestAssignment(char *line, char *key, size_t keyCapacity,
                                  char *value, size_t valueCapacity) {
    const char *cursor = ManifestTrim(line);
    if (!ManifestString(&cursor, key, keyCapacity)) return 0;
    while (isspace((unsigned char)*cursor)) cursor++;
    if (*cursor++ != '=') return 0;
    while (isspace((unsigned char)*cursor)) cursor++;
    if (!ManifestString(&cursor, value, valueCapacity)) return 0;
    return ManifestLineEnd(cursor);
}

static int ManifestRequirements(const char *cursor, RageModManifest *out) {
    while (isspace((unsigned char)*cursor)) ++cursor;
    if (*cursor++ != '=') return 0;
    while (isspace((unsigned char)*cursor)) ++cursor;
    if (*cursor++ != '[') return 0;
    for (;;) {
        while (isspace((unsigned char)*cursor)) ++cursor;
        if (*cursor == ']') return ManifestLineEnd(cursor + 1);
        if (out->requirementCount == RAGE_MOD_MANIFEST_MAX_REQUIREMENTS) return 0;
        char *id = out->requirements[out->requirementCount];
        if (!ManifestString(&cursor, id, RAGE_MOD_MANIFEST_ID_CAPACITY) ||
            !ManifestSemanticId(id)) return 0;
        for (size_t i = 0; i < out->requirementCount; ++i)
            if (strcmp(id, out->requirements[i]) == 0) return 0;
        ++out->requirementCount;
        while (isspace((unsigned char)*cursor)) ++cursor;
        if (*cursor == ']') return ManifestLineEnd(cursor + 1);
        if (*cursor++ != ',') return 0;
    }
}

int ModManifestParse(const char *text, size_t size, RageModManifest *out) {
    RageModSection section = RAGE_MOD_SECTION_NONE;
    size_t start = 0, lineNumber = 0, i;
    int versionSeen = 0, requirementsSeen = 0, idSeen = 0;
    RageModManifestError error = RAGE_MOD_MANIFEST_INVALID;
    if (out == NULL) return 0;
    memset(out, 0, sizeof(*out));
    if (text == NULL) goto invalid;
    out->schemaVersion = RAGE_MOD_MANIFEST_SCHEMA_VERSION;
    for (i = 0; i <= size; i++) {
        if (i == size || text[i] == '\n' || text[i] == '\r') {
            char buffer[1200];
            char *line;
            size_t length = i - start;
            lineNumber++;
            while (i + 1 < size && text[i] == '\r' && text[i + 1] == '\n')
                i++;
            if (length >= sizeof(buffer)) goto invalid;
            if (memchr(text + start, '\0', length) != NULL) goto invalid;
            memcpy(buffer, text + start, length);
            buffer[length] = '\0';
            line = ManifestTrim(buffer);
            if (*line == '\0' || *line == '#') goto next;
            if (strcmp(line, "[mod]") == 0) {
                section = RAGE_MOD_SECTION_MOD;
                goto next;
            }
            if (strcmp(line, "[textures]") == 0) {
                section = RAGE_MOD_SECTION_TEXTURES;
                goto next;
            }
            if (strcmp(line, "[materials]") == 0) {
                section = RAGE_MOD_SECTION_MATERIALS;
                goto next;
            }
            if (strcmp(line, "[meshes]") == 0) {
                section = RAGE_MOD_SECTION_MESHES;
                goto next;
            }
            if (*line == '[') {
                section = RAGE_MOD_SECTION_NONE;
                goto next;
            }
            if (section == RAGE_MOD_SECTION_MOD) {
                const char *cursor = line;
                if (strncmp(cursor, "requires", 8) == 0 &&
                    (cursor[8] == '=' || isspace((unsigned char)cursor[8]))) {
                    if (requirementsSeen || !ManifestRequirements(cursor + 8, out)) goto invalid;
                    requirementsSeen = 1;
                    goto next;
                }
                if (strncmp(cursor, "schema_version", 14) == 0 &&
                    (cursor[14] == '=' ||
                     isspace((unsigned char)cursor[14]))) {
                    unsigned version = 0;
                    if (versionSeen) goto invalid;
                    versionSeen = 1;
                    cursor += 14;
                    while (isspace((unsigned char)*cursor)) cursor++;
                    if (*cursor++ != '=') goto invalid;
                    while (isspace((unsigned char)*cursor)) cursor++;
                    if (*cursor < '0' || *cursor > '9') goto invalid;
                    do {
                        unsigned digit = (unsigned)(*cursor++ - '0');
                        if (version > (UINT_MAX - digit) / 10) goto invalid;
                        version = version * 10 + digit;
                    } while (*cursor >= '0' && *cursor <= '9');
                    if (!ManifestLineEnd(cursor)) goto invalid;
                    if (version != RAGE_MOD_MANIFEST_SCHEMA_VERSION) {
                        error = RAGE_MOD_MANIFEST_UNSUPPORTED_VERSION;
                        goto invalid;
                    }
                    out->schemaVersion = version;
                    goto next;
                }
                if (strncmp(cursor, "id", 2) != 0 ||
                    (cursor[2] != '=' &&
                     !isspace((unsigned char)cursor[2]))) goto next;
                if (idSeen) goto invalid;
                idSeen = 1;
                cursor += 2;
                while (isspace((unsigned char)*cursor)) cursor++;
                if (*cursor++ != '=') goto invalid;
                while (isspace((unsigned char)*cursor)) cursor++;
                if (!ManifestString(&cursor, out->id, sizeof(out->id)) ||
                    !ManifestSemanticId(out->id) ||
                    !ManifestLineEnd(cursor)) goto invalid;
            } else if (section == RAGE_MOD_SECTION_TEXTURES ||
                       section == RAGE_MOD_SECTION_MESHES) {
                RageModTextureOverride *entry;
                int mesh = section == RAGE_MOD_SECTION_MESHES;
                size_t *count = mesh ? &out->meshCount : &out->textureCount;
                if (*count == (size_t)(mesh ? RAGE_MOD_MANIFEST_MAX_MESHES :
                                            RAGE_MOD_MANIFEST_MAX_TEXTURES))
                    goto invalid;
                entry = mesh ? &out->meshes[*count] : &out->textures[*count];
                if (!ManifestAssignment(line, entry->key,
                                            sizeof(entry->key), entry->path,
                                            sizeof(entry->path)) ||
                    !ManifestSemanticId(entry->key) ||
                    !ManifestRelativePath(entry->path)) goto invalid;
                if (mesh) {
                    size_t length = strlen(entry->path);
                    if (strncmp(entry->path, "meshes/", 7) != 0 || length < 14 ||
                        strcmp(entry->path + length - 6, ".rmesh") != 0)
                        goto invalid;
                }
                (*count)++;
            } else if (section == RAGE_MOD_SECTION_MATERIALS) {
                RageModMaterialOverride *entry;
                RageRenderMaterial material;
                if (out->materialCount == RAGE_MOD_MANIFEST_MAX_MATERIALS)
                    goto invalid;
                entry = &out->materials[out->materialCount];
                if (!ManifestAssignment(
                        line, entry->key, sizeof(entry->key),
                        entry->properties, sizeof(entry->properties)) ||
                    !ManifestSemanticId(entry->key)) goto invalid;
                RenderMaterialDefault(&material);
                if (!RenderMaterialParseProperties(
                        entry->properties, strlen(entry->properties),
                        &material)) goto invalid;
                out->materialCount++;
            }
next:
            start = i + 1;
        }
    }
    return 1;
invalid:
    memset(out, 0, sizeof(*out));
    out->errorLine = lineNumber;
    out->error = error;
    return 0;
}

const char *ModManifestErrorString(RageModManifestError error) {
    switch (error) {
    case RAGE_MOD_MANIFEST_OK: return "no error";
    case RAGE_MOD_MANIFEST_UNSUPPORTED_VERSION:
        return "unsupported mod schema_version (supported: 1)";
    default: return "invalid mod manifest";
    }
}

const char *ModManifestFindMesh(const RageModManifest *manifest,
                               const char *semanticId) {
    RageModResolution result = ModManifestResolve(manifest, semanticId, NULL,
                                                 RAGE_MOD_RESOLVE_MESH);
    return result.mesh != NULL ? result.mesh->path : NULL;
}

const char *ModManifestFindMaterialProperties(
    const RageModManifest *manifest, const char *semanticId) {
    RageModResolution result = ModManifestResolve(manifest, semanticId, NULL,
                                                 RAGE_MOD_RESOLVE_MATERIAL);
    return result.material != NULL ? result.material->properties : NULL;
}

const char *ModManifestFindTexture(const RageModManifest *manifest,
                                      const char *semanticId) {
    RageModResolution result = ModManifestResolve(manifest, semanticId, NULL,
                                                 RAGE_MOD_RESOLVE_TEXTURE);
    return result.texture != NULL ? result.texture->path : NULL;
}

RageModResolution ModManifestResolve(const RageModManifest *manifest,
    const char *exactId, const char *baseId, unsigned channels) {
    RageModResolution result = {0};
    if (manifest == NULL || manifest->error != RAGE_MOD_MANIFEST_OK ||
        manifest->schemaVersion != RAGE_MOD_MANIFEST_SCHEMA_VERSION ||
        (channels & ~(RAGE_MOD_RESOLVE_TEXTURE | RAGE_MOD_RESOLVE_MATERIAL |
                      RAGE_MOD_RESOLVE_MESH)) != 0 ||
        manifest->textureCount > RAGE_MOD_MANIFEST_MAX_TEXTURES ||
        manifest->materialCount > RAGE_MOD_MANIFEST_MAX_MATERIALS ||
        manifest->meshCount > RAGE_MOD_MANIFEST_MAX_MESHES) return result;
    for (size_t i = (channels & RAGE_MOD_RESOLVE_TEXTURE) ? manifest->textureCount : 0; i > 0; --i) {
        const RageModTextureOverride *entry = &manifest->textures[i - 1];
        if (!memchr(entry->key, '\0', sizeof(entry->key)) ||
            !memchr(entry->path, '\0', sizeof(entry->path)))
            return (RageModResolution){0};
        if (exactId != NULL && strcmp(entry->key, exactId) == 0) {
            result.texture = entry;
            break;
        }
        if (result.texture == NULL && baseId != NULL && strcmp(entry->key, baseId) == 0)
            result.texture = entry;
    }
    for (size_t i = (channels & RAGE_MOD_RESOLVE_MATERIAL) ? manifest->materialCount : 0; i > 0; --i) {
        const RageModMaterialOverride *entry = &manifest->materials[i - 1];
        if (!memchr(entry->key, '\0', sizeof(entry->key)) ||
            !memchr(entry->properties, '\0', sizeof(entry->properties)))
            return (RageModResolution){0};
        if (exactId != NULL && strcmp(entry->key, exactId) == 0) {
            result.material = entry;
            break;
        }
        if (result.material == NULL && baseId != NULL && strcmp(entry->key, baseId) == 0)
            result.material = entry;
    }
    for (size_t i = (channels & RAGE_MOD_RESOLVE_MESH) ? manifest->meshCount : 0; i > 0; --i) {
        const RageModTextureOverride *entry = &manifest->meshes[i - 1];
        if (!memchr(entry->key, '\0', sizeof(entry->key)) ||
            !memchr(entry->path, '\0', sizeof(entry->path)))
            return (RageModResolution){0};
        if (exactId != NULL && strcmp(entry->key, exactId) == 0) {
            result.mesh = entry;
            break;
        }
        if (result.mesh == NULL && baseId != NULL && strcmp(entry->key, baseId) == 0)
            result.mesh = entry;
    }
    return result;
}

int ModManifestBuildOrder(const RageModManifest *const *manifests,
                          size_t count, RageModOrder *out) {
    RageModSelectionEntry entries[RAGE_MOD_MAX_SELECTED] = {0};
    RageModDependency dependencies[RAGE_MOD_MAX_SELECTED][RAGE_MOD_MANIFEST_MAX_REQUIREMENTS];
    RageModSelectionOrder selection;
    if (out == NULL) return 0;
    memset(out, 0, sizeof(*out));
    out->error = RAGE_MOD_ORDER_INVALID;
    out->modIndex = out->requirementIndex = (size_t)-1;
    if (count > RAGE_MOD_MAX_SELECTED || (count && manifests == NULL)) return 0;
    for (size_t i = 0; i < count; ++i) {
        const RageModManifest *m = manifests[i];
        out->modIndex = i;
        if (m == NULL || m->error != RAGE_MOD_MANIFEST_OK ||
            m->schemaVersion != RAGE_MOD_MANIFEST_SCHEMA_VERSION ||
            m->textureCount > RAGE_MOD_MANIFEST_MAX_TEXTURES ||
            m->materialCount > RAGE_MOD_MANIFEST_MAX_MATERIALS ||
            m->meshCount > RAGE_MOD_MANIFEST_MAX_MESHES ||
            m->requirementCount > RAGE_MOD_MANIFEST_MAX_REQUIREMENTS ||
            (count > 1 && m->id[0] == 0)) return 0;
        if (!memchr(m->id, '\0', sizeof(m->id)) ||
            (m->id[0] && !ManifestSemanticId(m->id))) return 0;
        for (size_t r = 0; r < m->requirementCount; ++r) {
            if (!memchr(m->requirements[r], '\0', sizeof(m->requirements[r])) ||
                !ManifestSemanticId(m->requirements[r])) {
                out->requirementIndex = r;
                return 0;
            }
        }
        for (size_t j = 0; j < i; ++j) {
            if (!strcmp(m->id, manifests[j]->id)) {
                out->error = RAGE_MOD_ORDER_DUPLICATE_ID;
                return 0;
            }
        }
    }
    for (size_t i = 0; i < count; ++i) {
        entries[i].manifestId = manifests[i]->id;
        entries[i].region = "";
        entries[i].dependencies = dependencies[i];
        entries[i].dependencyCount = manifests[i]->requirementCount;
        for (size_t r = 0; r < manifests[i]->requirementCount; ++r)
            dependencies[i][r] = (RageModDependency){
                RAGE_MOD_MANIFEST_ID, manifests[i]->requirements[r], NULL};
    }
    if (!ModSelectionBuildOrder(entries, count, &selection)) {
        out->modIndex = selection.modIndex;
        out->requirementIndex = selection.dependencyIndex;
        out->error = selection.code == RAGE_MOD_SELECTION_CYCLE ? RAGE_MOD_ORDER_CYCLE
            : selection.code == RAGE_MOD_SELECTION_MISSING ? RAGE_MOD_ORDER_MISSING_REQUIREMENT
            : RAGE_MOD_ORDER_INVALID;
        return 0;
    }
    out->count = selection.count;
    memcpy(out->indices, selection.indices, selection.count * sizeof(out->indices[0]));
    out->error = RAGE_MOD_ORDER_OK;
    out->modIndex = out->requirementIndex = (size_t)-1;
    return 1;
}

const char *ModManifestOrderErrorString(RageModOrderError error) {
    switch (error) {
    case RAGE_MOD_ORDER_OK: return "no error";
    case RAGE_MOD_ORDER_DUPLICATE_ID: return "duplicate mod ID";
    case RAGE_MOD_ORDER_MISSING_REQUIREMENT: return "missing required mod";
    case RAGE_MOD_ORDER_CYCLE: return "mod dependency cycle";
    default: return "invalid mod selection";
    }
}
