#include "disc_cue.h"
#include "disc_iso.h"
#include "disc_stage_validation.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define mkdir(path, mode) _mkdir(path)
#define strcasecmp _stricmp
#else
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

enum { PATH_CAPACITY = 4096, RAW_SECTOR_BYTES = 2352 };

typedef struct StageVersion {
    const char *name;
    const char *serial;
    const char *exeName;
    const char *sha1;
} StageVersion;

typedef struct RawFile {
    FILE *file;
    long offset;
} RawFile;

static const StageVersion s_versions[] = {
    {"PAL", "SCES-006.50", "SCES_006.50",
     "2913e15648eddef40821c5f666460abc04155ee6"},
    {"USA", "SLUS-004.03", "SLUS_004.03",
     "2661e8bf18d209c98fd70d33e18ab88b10abd52b"},
};

static void Usage(const char *program) {
    fprintf(stderr, "usage: %s [--root PATH] --pal-cue PATH [--usa-cue PATH]\n",
            program);
}

static const char *BaseName(const char *path) {
    const char *slash = strrchr(path, '/');
    const char *backslash = strrchr(path, '\\');
    if (backslash && (!slash || backslash > slash)) slash = backslash;
    return slash ? slash + 1 : path;
}

static int DirectoryName(const char *path, char output[PATH_CAPACITY]) {
    const char *base = BaseName(path);
    size_t length = (size_t)(base - path);
    if (length == 0) {
        strcpy(output, ".");
        return 1;
    }
    if (length >= PATH_CAPACITY) return 0;
    memcpy(output, path, length);
    output[length] = '\0';
    return 1;
}

static int JoinPath(char output[PATH_CAPACITY], const char *left,
                    const char *right) {
    size_t leftLength = strlen(left), rightLength = strlen(right);
    int separator = leftLength != 0 && left[leftLength - 1] != '/' &&
                    left[leftLength - 1] != '\\';
    if (leftLength + separator + rightLength >= PATH_CAPACITY) return 0;
    memcpy(output, left, leftLength);
    if (separator) output[leftLength++] = '/';
    memcpy(output + leftLength, right, rightLength + 1);
    return 1;
}

static int AbsolutePath(const char *input, char output[PATH_CAPACITY]) {
#ifdef _WIN32
    return _fullpath(output, input, PATH_CAPACITY) != NULL;
#else
    return realpath(input, output) != NULL;
#endif
}

static int MakeDirectories(const char *path) {
    char copy[PATH_CAPACITY];
    char *cursor;
    size_t length = strlen(path);
    if (length >= sizeof(copy)) return 0;
    memcpy(copy, path, length + 1);
    for (cursor = copy + 1; *cursor; ++cursor) {
        if (*cursor != '/' && *cursor != '\\') continue;
        *cursor = '\0';
        if (copy[0] && mkdir(copy, 0777) != 0 && errno != EEXIST) return 0;
        *cursor = '/';
    }
    return !copy[0] || mkdir(copy, 0777) == 0 || errno == EEXIST;
}

static int ForceSymlink(const char *source, const char *destination) {
#ifdef _WIN32
    if (DeleteFileA(destination) == 0 && GetLastError() != ERROR_FILE_NOT_FOUND)
        return 0;
    return CreateSymbolicLinkA(destination, source, 0) != 0;
#else
    if (unlink(destination) != 0 && errno != ENOENT) return 0;
    return symlink(source, destination) == 0;
#endif
}

static int ParseFileName(char *line, char output[PATH_CAPACITY]) {
    char *cursor = line;
    char *end;
    size_t length;
    while (isspace((unsigned char)*cursor)) cursor++;
    if (*cursor == '"') {
        ++cursor;
        end = strchr(cursor, '"');
    } else {
        end = cursor;
        while (*end && !isspace((unsigned char)*end)) ++end;
    }
    if (!end || end == cursor) return 0;
    length = (size_t)(end - cursor);
    if (length >= PATH_CAPACITY) return 0;
    memcpy(output, cursor, length);
    output[length] = '\0';
    return 1;
}

static int StageCueFiles(const char *cue, const char *discDirectory) {
    FILE *file;
    char cueDirectory[PATH_CAPACITY], source[PATH_CAPACITY];
    char destination[PATH_CAPACITY], line[PATH_CAPACITY], name[PATH_CAPACITY];
    if (!DirectoryName(cue, cueDirectory) ||
        !JoinPath(destination, discDirectory, BaseName(cue)) ||
        !ForceSymlink(cue, destination)) return 0;
    file = fopen(cue, "r");
    if (!file) return 0;
    while (fgets(line, sizeof(line), file)) {
        char *cursor = line;
        FILE *track;
        while (isspace((unsigned char)*cursor)) cursor++;
        if (strncasecmp(cursor, "FILE", 4) != 0 ||
            !isspace((unsigned char)cursor[4]) ||
            !ParseFileName(cursor + 4, name)) continue;
        if (!JoinPath(source, cueDirectory, name)) {
            fclose(file);
            return 0;
        }
        track = fopen(source, "rb");
        if (!track) { fclose(file); return 0; }
        fclose(track);
        if (!JoinPath(destination, discDirectory, BaseName(name)) ||
            !ForceSymlink(source, destination)) {
            fclose(file);
            return 0;
        }
    }
    fclose(file);
    return 1;
}

static int ReadRawSector(void *context, unsigned int sector, unsigned char *raw) {
    RawFile *source = context;
    long offset;
    if (!source || !source->file || !raw ||
        sector > (unsigned long)(LONG_MAX - source->offset) / RAW_SECTOR_BYTES)
        return 0;
    offset = source->offset + (long)sector * RAW_SECTOR_BYTES;
    return fseek(source->file, offset, SEEK_SET) == 0 &&
           fread(raw, 1, RAW_SECTOR_BYTES, source->file) == RAW_SECTOR_BYTES;
}

static int WriteFile(const char *path, const unsigned char *bytes, size_t size) {
    FILE *file = fopen(path, "wb");
    int ok;
    if (!file) return 0;
    ok = fwrite(bytes, 1, size, file) == size && fclose(file) == 0;
    if (!ok) remove(path);
    return ok;
}

static uint32_t ReadLe32(const unsigned char *bytes) {
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

static int ExtractFiles(const StageVersion *version, const char *cue,
                        const char *assetDirectory) {
    char rawPath[PATH_CAPACITY], mainPath[PATH_CAPACITY], configPath[PATH_CAPACITY];
    long offset;
    RawFile raw = {0};
    DiscIsoReader iso;
    DiscIsoFile exeFile, configFile;
    unsigned char *exe = NULL, *config = NULL;
    int ok = 0;
    if (!DiscCueResolveDataTrack(cue, rawPath, sizeof(rawPath), &offset) ||
        offset < 0 || !MakeDirectories(assetDirectory) ||
        !JoinPath(mainPath, assetDirectory, "main.exe") ||
        !JoinPath(configPath, assetDirectory, "SYSTEM.CNF")) return 0;
    raw.file = fopen(rawPath, "rb");
    raw.offset = offset;
    if (!raw.file || !DiscIsoOpen(&iso, ReadRawSector, &raw) ||
        !DiscIsoFindFile(&iso, version->exeName, &exeFile) ||
        !DiscIsoFindFile(&iso, "SYSTEM.CNF", &configFile)) goto done;
    exe = DiscIsoReadWholeFile(&iso, &exeFile);
    config = DiscIsoReadWholeFile(&iso, &configFile);
    if (!exe || !config || !RageDiscStageValidatePsxExe(exe, exeFile.size,
                                                          version->sha1) ||
        !WriteFile(mainPath, exe, exeFile.size) ||
        !WriteFile(configPath, config, configFile.size)) goto done;
    printf("%s: %s staged, SHA-1 %s, entry=0x%08X text=0x%08X/0x%X\n",
           version->name, version->serial, version->sha1, ReadLe32(exe + 0x10),
           ReadLe32(exe + 0x18), ReadLe32(exe + 0x1c));
    ok = 1;
done:
    free(exe);
    free(config);
    if (raw.file) fclose(raw.file);
    return ok;
}

static int Stage(const StageVersion *version, const char *cue, const char *root) {
    char discDirectory[PATH_CAPACITY], assetDirectory[PATH_CAPACITY];
    char discRoot[PATH_CAPACITY], assetRoot[PATH_CAPACITY];
    char absoluteCue[PATH_CAPACITY];
    if (!cue || !*cue || !AbsolutePath(cue, absoluteCue) ||
        !JoinPath(discRoot, root, "disc") ||
        !JoinPath(discDirectory, discRoot, version->name) ||
        !JoinPath(assetRoot, root, "assets") ||
        !JoinPath(assetDirectory, assetRoot, version->name) ||
        !MakeDirectories(discDirectory) ||
        !StageCueFiles(absoluteCue, discDirectory) ||
        !ExtractFiles(version, absoluteCue, assetDirectory)) {
        fprintf(stderr, "%s: could not stage %s: %s\n", version->name, cue,
                strerror(errno));
        return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    const char *root = RAGE_STAGE_SOURCE_ROOT;
    const char *palCue = NULL, *usaCue = NULL;
    int index;
    for (index = 1; index < argc; ++index) {
        if (strcmp(argv[index], "--root") == 0 && index + 1 < argc) root = argv[++index];
        else if (strcmp(argv[index], "--pal-cue") == 0 && index + 1 < argc) palCue = argv[++index];
        else if (strcmp(argv[index], "--usa-cue") == 0 && index + 1 < argc) usaCue = argv[++index];
        else { Usage(argv[0]); return 2; }
    }
    if (!palCue && !usaCue) { Usage(argv[0]); return 2; }
    return (!palCue || Stage(&s_versions[0], palCue, root)) &&
           (!usaCue || Stage(&s_versions[1], usaCue, root)) ? 0 : 1;
}
