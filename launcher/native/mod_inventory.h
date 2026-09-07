#ifndef RAGE_MOD_INVENTORY_H
#define RAGE_MOD_INVENTORY_H
#include "render/mod_file_policy.h"
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <sys/stat.h>
#endif

typedef struct ModInventory {
    char *files[10000];
    size_t count, directories, pathBytes;
    Uint64 bytes;
} ModInventory;
typedef struct ModInventoryWalk {
    ModInventory *inventory;
    const char *relative;
} ModInventoryWalk;

/* SDL path info follows symlinks. Reject links/reparse points first, using
 * UTF-16 on Windows so package roots retain their UTF-8 CLI spelling. This
 * is a check, not an atomic filesystem snapshot against concurrent edits. */
static int InventoryNotLink(const char *path) {
#ifdef _WIN32
    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, NULL, 0);
    if (count <= 0) return 0;
    wchar_t *wide = malloc((size_t)count * sizeof(*wide));
    if (!wide) return 0;
    DWORD flags = INVALID_FILE_ATTRIBUTES;
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, count))
        flags = GetFileAttributesW(wide);
    free(wide);
    return flags != INVALID_FILE_ATTRIBUTES && !(flags & FILE_ATTRIBUTE_REPARSE_POINT);
#else
    struct stat info;
    return lstat(path, &info) == 0 && !S_ISLNK(info.st_mode);
#endif
}

static SDL_EnumerationResult SDLCALL InventoryEntry(void *context,
                                                   const char *directory,
                                                   const char *name) {
    ModInventoryWalk *walk = context;
    ModInventory *inventory = walk->inventory;
    char *relative = NULL, *full = NULL;
    SDL_PathInfo info;
    int ok = 0;
    if (name[0] == '.') return SDL_ENUM_CONTINUE;
    if (SDL_asprintf(&relative, "%s%s%s", walk->relative,
                     walk->relative[0] ? "/" : "", name) < 0 ||
        SDL_asprintf(&full, "%s%s", directory, name) < 0) goto done;
    if (!InventoryNotLink(full) || !SDL_GetPathInfo(full, &info)) goto done;
    if (info.type == SDL_PATHTYPE_DIRECTORY) {
        if (!ModDirectoryAllowed(relative) || inventory->directories >= 10000) goto done;
        ++inventory->directories;
        ModInventoryWalk child = {inventory, relative};
        ok = SDL_EnumerateDirectory(full, InventoryEntry, &child);
    } else if (info.type == SDL_PATHTYPE_FILE) {
        size_t length = strlen(relative) + 1;
        if (ModFileClassify(relative) == RAGE_MOD_FILE_INVALID ||
            inventory->count >= 10000 || info.size > 128u * 1024u * 1024u ||
            info.size > 1024u * 1024u * 1024u - inventory->bytes ||
            length > 8u * 1024u * 1024u - inventory->pathBytes) goto done;
        inventory->bytes += info.size;
        inventory->pathBytes += length;
        inventory->files[inventory->count++] = relative;
        relative = NULL;
        ok = 1;
    }
done:
    if (!ok) fprintf(stderr, "Unsupported mod file: %s\n", relative ? relative : name);
    SDL_free(relative);
    SDL_free(full);
    return ok ? SDL_ENUM_CONTINUE : SDL_ENUM_FAILURE;
}

static int InventoryCompare(const void *a, const void *b) {
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}
static int InventoryCommand(const char *root) {
    ModInventory *inventory = calloc(1, sizeof(*inventory));
    if (!inventory) return 1;
    ModInventoryWalk walk = {inventory, ""};
    int ok = SDL_EnumerateDirectory(root, InventoryEntry, &walk) && inventory->count > 0;
    if (ok) {
        qsort(inventory->files, inventory->count, sizeof(*inventory->files), InventoryCompare);
        putchar('[');
        for (size_t i = 0; i < inventory->count; ++i)
            /* Validated package paths contain no JSON quoting characters. */
            printf("%s\"%s\"", i ? "," : "", inventory->files[i]);
        puts("]");
        ok = !ferror(stdout);
    } else {
        fputs("Invalid or empty mod file inventory\n", stderr);
    }
    for (size_t i = 0; i < inventory->count; ++i) SDL_free(inventory->files[i]);
    free(inventory);
    return ok ? 0 : 1;
}
#endif
