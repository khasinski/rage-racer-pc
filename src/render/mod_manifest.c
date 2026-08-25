#include "mod_manifest.h"

#include <ctype.h>
#include <string.h>

typedef enum RageModSection {
    RAGE_MOD_SECTION_NONE,
    RAGE_MOD_SECTION_MOD,
    RAGE_MOD_SECTION_TEXTURES,
} RageModSection;

static char *RageManifestTrim(char *text) {
    char *end;
    while (isspace((unsigned char)*text)) text++;
    end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) end--;
    *end = '\0';
    return text;
}

static int RageManifestString(const char **cursor, char *out,
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

static int RageManifestLineEnd(const char *cursor) {
    while (isspace((unsigned char)*cursor)) cursor++;
    return *cursor == '\0' || *cursor == '#';
}

static int RageManifestRelativePath(const char *path) {
    const char *component = path;
    if (*path == '\0' || *path == '/' || *path == '\\' ||
        strchr(path, ':') != NULL || strchr(path, '\\') != NULL) return 0;
    while (*component != '\0') {
        const char *end = strchr(component, '/');
        size_t length = end != NULL ? (size_t)(end - component)
                                    : strlen(component);
        if (length == 0 || (length == 1 && component[0] == '.') ||
            (length == 2 && component[0] == '.' && component[1] == '.'))
            return 0;
        if (end == NULL) break;
        component = end + 1;
    }
    return 1;
}

static int RageManifestSemanticId(const char *key) {
    const unsigned char *at = (const unsigned char *)key;
    if (*at == '\0') return 0;
    while (*at != '\0') {
        if (!islower(*at) && !isdigit(*at) && *at != '.' && *at != '-')
            return 0;
        at++;
    }
    return 1;
}

static int RageManifestAssignment(char *line, char *key, size_t keyCapacity,
                                  char *value, size_t valueCapacity) {
    const char *cursor = RageManifestTrim(line);
    if (!RageManifestString(&cursor, key, keyCapacity)) return 0;
    while (isspace((unsigned char)*cursor)) cursor++;
    if (*cursor++ != '=') return 0;
    while (isspace((unsigned char)*cursor)) cursor++;
    if (!RageManifestString(&cursor, value, valueCapacity)) return 0;
    return RageManifestLineEnd(cursor);
}

int RageModManifestParse(const char *text, size_t size, RageModManifest *out) {
    RageModSection section = RAGE_MOD_SECTION_NONE;
    size_t start = 0, lineNumber = 0, i;
    if (text == NULL || out == NULL) return 0;
    memset(out, 0, sizeof(*out));
    for (i = 0; i <= size; i++) {
        if (i == size || text[i] == '\n' || text[i] == '\r') {
            char buffer[1200];
            char *line;
            size_t length = i - start;
            lineNumber++;
            while (i + 1 < size && text[i] == '\r' && text[i + 1] == '\n')
                i++;
            if (length >= sizeof(buffer)) goto invalid;
            memcpy(buffer, text + start, length);
            buffer[length] = '\0';
            line = RageManifestTrim(buffer);
            if (*line == '\0' || *line == '#') goto next;
            if (strcmp(line, "[mod]") == 0) {
                section = RAGE_MOD_SECTION_MOD;
                goto next;
            }
            if (strcmp(line, "[textures]") == 0) {
                section = RAGE_MOD_SECTION_TEXTURES;
                goto next;
            }
            if (*line == '[') {
                section = RAGE_MOD_SECTION_NONE;
                goto next;
            }
            if (section == RAGE_MOD_SECTION_MOD) {
                const char *cursor = line;
                if (strncmp(cursor, "id", 2) != 0 ||
                    !isspace((unsigned char)cursor[2])) goto next;
                cursor += 2;
                while (isspace((unsigned char)*cursor)) cursor++;
                if (*cursor++ != '=') goto invalid;
                while (isspace((unsigned char)*cursor)) cursor++;
                if (!RageManifestString(&cursor, out->id, sizeof(out->id)) ||
                    !RageManifestLineEnd(cursor)) goto invalid;
            } else if (section == RAGE_MOD_SECTION_TEXTURES) {
                RageModTextureOverride *entry;
                if (out->textureCount == RAGE_MOD_MANIFEST_MAX_TEXTURES)
                    goto invalid;
                entry = &out->textures[out->textureCount];
                if (!RageManifestAssignment(line, entry->key,
                                            sizeof(entry->key), entry->path,
                                            sizeof(entry->path)) ||
                    !RageManifestSemanticId(entry->key) ||
                    !RageManifestRelativePath(entry->path)) goto invalid;
                out->textureCount++;
            }
next:
            start = i + 1;
        }
    }
    return 1;
invalid:
    out->errorLine = lineNumber;
    return 0;
}

const char *RageModManifestFindTexture(const RageModManifest *manifest,
                                      const char *semanticId) {
    size_t index;
    if (manifest == NULL || semanticId == NULL) return NULL;
    /* Later entries deliberately override earlier ones, matching TOML/config
     * expectations while keeping lookup allocation-free. */
    for (index = manifest->textureCount; index > 0; index--)
        if (strcmp(manifest->textures[index - 1].key, semanticId) == 0)
            return manifest->textures[index - 1].path;
    return NULL;
}
