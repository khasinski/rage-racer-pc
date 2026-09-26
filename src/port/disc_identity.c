#include "disc_identity.h"
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
int _strnicmp(const char *lhs, const char *rhs, unsigned long long count);
#define strncasecmp _strnicmp
#else
#include <strings.h>
#endif

/* "SLUS_004.03": four letters, an underscore, then three digits, a dot and two
 * more.  Nothing else in a PlayStation root directory looks like that. */
static int LooksLikeSerial(const unsigned char *name, unsigned int length) {
    unsigned int index;
    if (length < 11) return 0;
    for (index = 0; index < 4; index++) {
        if (name[index] < 'A' || name[index] > 'Z') return 0;
    }
    if (name[4] != '_' || name[8] != '.') return 0;
    for (index = 5; index < 11; index++) {
        if (index == 8) continue;
        if (name[index] < '0' || name[index] > '9') return 0;
    }
    return length == 11 || name[11] == ';';
}

typedef struct BootNameSearch {
    char *boot;
    int found;
} BootNameSearch;

static int FindBootNameVisitor(void *context, const unsigned char *name,
                               unsigned int length,
                               const DiscIsoFile *file) {
    BootNameSearch *search = context;
    (void)file;

    if (!LooksLikeSerial(name, length)) return 0;
    memcpy(search->boot, name, 11);
    search->boot[11] = '\0';
    search->found = 1;
    return 1;
}

static int FindBootLikeName(DiscIsoReader *reader, char *boot, size_t bootSize) {
    BootNameSearch search;

    if (bootSize < 12) return 0;
    search.boot = boot;
    search.found = 0;
    return DiscIsoVisitRoot(reader, FindBootNameVisitor, &search) &&
           search.found;
}

/* SYSTEM.CNF's BOOT line names the executable, which is the disc's serial:
 * "BOOT = cdrom:\SLUS_004.03;1". */
static int ParseBootName(const unsigned char *cnf, unsigned int size,
                         char *boot, size_t bootSize) {
    unsigned int index;
    for (index = 0; index + 4 <= size; index++) {
        unsigned int cursor;
        size_t written = 0;
        if (strncasecmp((const char *)cnf + index, "BOOT", 4) != 0) continue;
        if (index != 0 && cnf[index - 1] != '\n' && cnf[index - 1] != '\r')
            continue;
        cursor = index + 4;
        while (cursor < size && (cnf[cursor] == ' ' || cnf[cursor] == '\t'))
            cursor++;
        if (cursor >= size || cnf[cursor] != '=') continue;
        cursor++;
        while (cursor < size && (cnf[cursor] == ' ' || cnf[cursor] == '\t'))
            cursor++;
        /* Step over "cdrom:" and any path separators before the name. */
        if (cursor + 6 <= size &&
            strncasecmp((const char *)cnf + cursor, "cdrom:", 6) == 0)
            cursor += 6;
        while (cursor < size && (cnf[cursor] == '\\' || cnf[cursor] == '/'))
            cursor++;
        while (cursor < size && cnf[cursor] > ' ' && cnf[cursor] != ';') {
            if (written + 1 >= bootSize) return 0;
            boot[written++] = (char)cnf[cursor++];
        }
        boot[written] = '\0';
        return written > 0;
    }
    return 0;
}

int DiscReadBootName(DiscIsoReader *reader, char *boot, size_t bootSize) {
    DiscIsoFile config;
    unsigned char *cnf;
    char name[16] = {0};
    int named = 0;
    if (reader == NULL || boot == NULL || bootSize == 0) return 0;
    if (DiscIsoFindFile(reader, "SYSTEM.CNF", &config)) {
        cnf = DiscIsoReadWholeFile(reader, &config);
        if (cnf != NULL) {
            named = ParseBootName(cnf, config.size, name, sizeof(name));
            free(cnf);
        }
    }
    /* A disc with no readable SYSTEM.CNF still names its executable in the
     * root directory, and a PlayStation serial has a shape of its own. */
    if (!named) named = FindBootLikeName(reader, name, sizeof(name));
    if (!named || strlen(name) >= bootSize) return 0;
    memcpy(boot, name, strlen(name) + 1);
    return 1;
}

