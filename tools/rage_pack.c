/*
 * rage-pack: write edited PNGs back into the raw assets rage-extract wrote.
 *
 * The game applies the same edits in memory as it loads, so this only exists
 * for looking at the result, or for shipping a mod as packed assets rather than
 * as images. Both go through texture_patch.c, so packing means one thing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

#include "texture_patch.h"

/* Publish only a complete, successfully closed file. An existing staging
 * path is never overwritten or removed: it may belong to another process. */
static int WriteAsset(const char *path, const void *data, size_t size) {
    char staging[1100];
    int length = snprintf(staging, sizeof(staging), "%s.rage-pack.tmp", path);
    FILE *file;
    int ok;
    if (length < 0 || (size_t)length >= sizeof(staging)) return 0;
    file = fopen(staging, "wbx");
    if (file == NULL) return 0;
    ok = fwrite(data, 1, size, file) == size;
    if (fclose(file) != 0) ok = 0;
    if (ok) {
#ifdef _WIN32
        ok = MoveFileExA(staging, path, MOVEFILE_REPLACE_EXISTING) != 0;
#else
        ok = rename(staging, path) == 0;
#endif
    }
    if (!ok) remove(staging);
    return ok;
}

int main(int argc, char **argv) {
    const char *directory;
    char indexPath[1024], line[512];
    FILE *index;
    int seen[512];
    int changed = 0, assets = 0, failed = 0, i;

    if (argc != 2) {
        fprintf(stderr, "usage: rage-pack <mod directory>\n");
        return 2;
    }
    directory = argv[1];
    memset(seen, 0, sizeof(seen));

    snprintf(indexPath, sizeof(indexPath), "%s/textures/index.txt", directory);
    index = fopen(indexPath, "rb");
    if (index == NULL) {
        fprintf(stderr,
                "rage-pack: %s has no textures/index.txt; run rage-extract into it first\n",
                directory);
        return 1;
    }
    while (fgets(line, sizeof(line), index)) {
        int owner;
        char stem[256];
        if (sscanf(line, "%d %255s", &owner, stem) != 2) continue;
        if (owner >= 0 && owner < (int)(sizeof(seen) / sizeof(seen[0])))
            seen[owner] = 1;
    }
    fclose(index);

    for (i = 0; i < (int)(sizeof(seen) / sizeof(seen[0])); i++) {
        char rawPath[1024];
        FILE *raw;
        unsigned char *data;
        long size;
        int patched;

        if (!seen[i]) continue;
        snprintf(rawPath, sizeof(rawPath), "%s/raw/asset_%03d.bin", directory, i);
        raw = fopen(rawPath, "rb");
        if (raw == NULL) {
            fprintf(stderr, "rage-pack: %s cannot be read\n", rawPath);
            failed = 1;
            continue;
        }
        if (fseek(raw, 0, SEEK_END) != 0) {
            fclose(raw);
            failed = 1;
            continue;
        }
        size = ftell(raw);
        if (fseek(raw, 0, SEEK_SET) != 0) size = -1;
        data = size > 0 ? malloc((size_t)size) : NULL;
        if (data == NULL || fread(data, 1, (size_t)size, raw) != (size_t)size) {
            fprintf(stderr, "rage-pack: %s cannot be read\n", rawPath);
            fclose(raw);
            free(data);
            failed = 1;
            continue;
        }
        fclose(raw);
        assets++;

        patched = TexturePatchAsset(directory, i, data, (size_t)size);
        if (patched > 0) {
            if (!WriteAsset(rawPath, data, (size_t)size)) {
                fprintf(stderr, "rage-pack: %s cannot be replaced; original preserved\n", rawPath);
                free(data);
                failed = 1;
                continue;
            }
            changed += patched;
        }
        free(data);
    }

    printf("rage-pack: %d textures written across %d assets\n", changed, assets);
    return failed ? 1 : 0;
}
