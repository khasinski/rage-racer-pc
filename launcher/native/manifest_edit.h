#ifndef RAGE_LAUNCHER_MANIFEST_EDIT_H
#define RAGE_LAUNCHER_MANIFEST_EDIT_H

/* Material edits preserve the original document, including schema, identity,
 * requirements and unknown extension fields. The parser's last-assignment
 * rule makes the appended entry authoritative. The caller supplies a new
 * staging path and owns the eventual transactional install/rollback. */
static char *ManifestEditQuote(char *out, const char *text) {
    *out++ = '"';
    while (*text) {
        if (*text == '"' || *text == '\\') *out++ = '\\';
        *out++ = *text++;
    }
    *out++ = '"';
    return out;
}

static int ManifestEditMaterial(const char *input, const char *output,
                                const char *key, const char *properties) {
    enum { LIMIT = 2 * 1024 * 1024 };
    char suffix[2 * (RAGE_MOD_MANIFEST_KEY_CAPACITY +
                     RAGE_MOD_MANIFEST_PROPERTIES_CAPACITY) + 32];
    char *cursor = suffix, *bytes;
    size_t size = 0, extra;
    FILE *file;
    int ok;
    if (strlen(key) >= RAGE_MOD_MANIFEST_KEY_CAPACITY ||
        strlen(properties) >= RAGE_MOD_MANIFEST_PROPERTIES_CAPACITY) return 1;
    memcpy(cursor, "\n[materials]\n", 13); cursor += 13;
    cursor = ManifestEditQuote(cursor, key);
    *cursor++ = '=';
    cursor = ManifestEditQuote(cursor, properties);
    *cursor++ = '\n';
    extra = (size_t)(cursor - suffix);
    bytes = malloc(LIMIT + 1u);
    if (!bytes) return 1;
    /* Empty input path explicitly requests a new manifest for a raw-only mod.
     * A missing nonempty path must never silently discard an existing mod. */
    if (*input) {
        file = fopen(input, "rb");
        if (!file) { perror(input); free(bytes); return 1; }
        size = fread(bytes, 1, LIMIT + 1u, file);
        ok = !ferror(file);
        if (fclose(file)) ok = 0;
        if (!ok || size > LIMIT) { free(bytes); return 1; }
    }
    if (!ModManifestParse(bytes, size, &manifest)) {
        fprintf(stderr, "Invalid source manifest at line %zu\n", manifest.errorLine);
        free(bytes); return 1;
    }
    if (size > LIMIT - extra) { free(bytes); return 1; }
    memcpy(bytes + size, suffix, extra);
    size += extra;
    if (!ModManifestParse(bytes, size, &manifest) ||
        !ModManifestFindMaterialProperties(&manifest, key)) {
        fputs("Invalid material edit\n", stderr);
        free(bytes); return 1;
    }
    file = fopen(output, "wbx");
    if (!file) { perror(output); free(bytes); return 1; }
    ok = fwrite(bytes, 1, size, file) == size;
    if (fclose(file)) ok = 0;
    free(bytes);
    if (!ok) remove(output);
    return ok ? 0 : 1;
}

#endif
