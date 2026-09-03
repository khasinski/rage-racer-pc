#include "disc_cue.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
int _strnicmp(const char *lhs, const char *rhs, unsigned long long count);
#define strncasecmp _strnicmp
#else
#include <strings.h>
#endif

enum {
    CUE_LINE_SIZE = 4160,
    CUE_PATH_SIZE = 4096,
    RAW_SECTOR_SIZE = 2352,
};

static char *SkipSpace(char *text) {
    while (isspace((unsigned char)*text)) text++;
    return text;
}

static int IsDirective(const char *line, const char *directive) {
    size_t length = strlen(directive);

    return strlen(line) > length &&
           strncasecmp(line, directive, length) == 0 &&
           isspace((unsigned char)line[length]);
}

static int ContainsIgnoreCase(const char *text, const char *wanted) {
    size_t length = strlen(wanted);

    while (*text != '\0') {
        if (strncasecmp(text, wanted, length) == 0) return 1;
        text++;
    }
    return 0;
}

static int ParseFileName(char *line, char *name, size_t size) {
    char *begin = SkipSpace(line);
    char *end;
    size_t length;

    if (*begin == '"') {
        begin++;
        end = strchr(begin, '"');
    } else {
        end = begin;
        while (*end != '\0' && !isspace((unsigned char)*end)) end++;
    }
    if (end == NULL || end == begin) return 0;
    length = (size_t)(end - begin);
    if (length >= size) return 0;
    memcpy(name, begin, length);
    name[length] = '\0';
    return 1;
}

static int ParseIndex(const char *line, long *track_offset) {
    int index;
    int minute;
    int second;
    int frame;
    int consumed = 0;
    long sectors;
    long sector_tail;

    if (sscanf(line, "%d %d:%d:%d%n", &index, &minute, &second, &frame,
               &consumed) != 4 ||
        index != 1 || minute < 0 || second < 0 || second >= 60 || frame < 0 ||
        frame >= 75) {
        return 0;
    }
    while (isspace((unsigned char)line[consumed])) consumed++;
    if (line[consumed] != '\0') return 0;
    sector_tail = (long)second * 75 + frame;
    if ((long)minute > (LONG_MAX - sector_tail) / (60 * 75)) return 0;
    sectors = (long)minute * 60 * 75 + sector_tail;
    if (sectors > LONG_MAX / RAW_SECTOR_SIZE) return 0;
    *track_offset = sectors * RAW_SECTOR_SIZE;
    return 1;
}

static int ResolveImagePath(const char *cue_path, const char *image_name,
                            char *image_path, size_t image_path_size) {
    const char *slash = strrchr(cue_path, '/');
    const char *backslash = strrchr(cue_path, '\\');
    size_t directory_length;
    size_t name_length = strlen(image_name);

    if (backslash != NULL && (slash == NULL || backslash > slash)) {
        slash = backslash;
    }
    if (image_name[0] == '/' || image_name[0] == '\\' ||
        (isalpha((unsigned char)image_name[0]) && image_name[1] == ':')) {
        if (name_length + 1 > image_path_size) return 0;
        memcpy(image_path, image_name, name_length + 1);
        return 1;
    }

    directory_length = slash != NULL ? (size_t)(slash - cue_path) : 1;
    if (directory_length + 1 + name_length + 1 > image_path_size) return 0;
    if (slash != NULL) {
        memcpy(image_path, cue_path, directory_length);
    } else {
        image_path[0] = '.';
    }
    image_path[directory_length] = '/';
    memcpy(image_path + directory_length + 1, image_name, name_length + 1);
    return 1;
}

int DiscCueResolveDataTrack(const char *cue_path, char *image_path,
                            size_t image_path_size, long *track_offset) {
    FILE *cue;
    char line[CUE_LINE_SIZE];
    char current_file[CUE_PATH_SIZE] = {0};
    char data_file[CUE_PATH_SIZE] = {0};
    int data_track = 0;
    int found = 0;

    if (cue_path == NULL || image_path == NULL || image_path_size == 0 ||
        track_offset == NULL) {
        return 0;
    }
    cue = fopen(cue_path, "r");
    if (cue == NULL) return 0;
    while (fgets(line, sizeof(line), cue) != NULL) {
        char *directive = SkipSpace(line);

        if (IsDirective(directive, "FILE")) {
            current_file[0] = '\0';
            ParseFileName(directive + 4, current_file,
                          sizeof(current_file));
        } else if (IsDirective(directive, "TRACK")) {
            data_track = ContainsIgnoreCase(directive, "MODE1/2352") ||
                         ContainsIgnoreCase(directive, "MODE2/2352");
        } else if (data_track && IsDirective(directive, "INDEX") &&
                   current_file[0] != '\0' &&
                   ParseIndex(SkipSpace(directive + 5), track_offset)) {
            memcpy(data_file, current_file, sizeof(data_file));
            found = 1;
            break;
        }
    }
    fclose(cue);
    return found && ResolveImagePath(cue_path, data_file, image_path,
                                     image_path_size);
}
