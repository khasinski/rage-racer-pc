#include "host_storage.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#define HOST_PATH_SEPARATOR '\\'
#else
#define HOST_PATH_SEPARATOR '/'
#endif
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#include <psyz.h>

#include "platform_paths.h"

static char s_MemoryCardDirectory[PATH_MAX];

static int AdjustMemoryCardPath(char *destination, const char *source,
                                int destinationSize) {
    const char *name;
    char card[5];
    int written;

    if (destination == NULL || source == NULL || destinationSize <= 0 ||
        strlen(source) < 5 || source[0] != 'b' || source[1] != 'u' ||
        source[4] != ':' ||
        !((source[2] == '0' || source[2] == '1') && source[3] == '0')) {
        return -1;
    }
    memcpy(card, source, 4);
    card[4] = '\0';
    name = source + 5;
    if (name[0] == '\0' || name[0] == '*') {
        written = snprintf(destination, (size_t)destinationSize, "%s%c%s%c",
                           s_MemoryCardDirectory, HOST_PATH_SEPARATOR, card,
                           HOST_PATH_SEPARATOR);
    } else {
        written = snprintf(destination, (size_t)destinationSize,
                           "%s%c%s%c%s", s_MemoryCardDirectory,
                           HOST_PATH_SEPARATOR, card, HOST_PATH_SEPARATOR,
                           name);
    }
    if (written < 0 || written >= destinationSize) {
        destination[0] = '\0';
        return -1;
    }
    return written;
}

int HostInitStorage(void) {
    char cardDirectory[PATH_MAX];
    char executableDirectory[PATH_MAX];
    int card;

    if (!(PlatformExecutableDirectory(
              NULL, executableDirectory, sizeof(executableDirectory)) &&
          PlatformExistingPortableStateDirectory(
              executableDirectory, s_MemoryCardDirectory,
              sizeof(s_MemoryCardDirectory))) &&
        !PlatformUserStateDirectory(s_MemoryCardDirectory,
                                    sizeof(s_MemoryCardDirectory))) {
        return 0;
    }
    if (!PlatformEnsureDirectory(s_MemoryCardDirectory)) return 0;

    for (card = 0; card < 2; card++) {
        int written = snprintf(cardDirectory, sizeof(cardDirectory),
                               "%s%cbu%d0", s_MemoryCardDirectory,
                               HOST_PATH_SEPARATOR, card);

        if (written < 0 || (size_t)written >= sizeof(cardDirectory) ||
            !PlatformEnsureDirectory(cardDirectory)) {
            return 0;
        }
    }
    Psyz_AdjustPathCB(AdjustMemoryCardPath);
    return 1;
}
