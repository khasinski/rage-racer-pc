#include "diagnostic_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "runtime_config.h"
#include "platform_paths.h"

int RageDiagnosticLogOpen(char *path, size_t pathSize) {
    const char *override = RageRuntimeConfigGetOverride(
        "diagnostics.log", "RAGE_PORT_LOG_PATH");
    char directory[1024];
    FILE *file;
    time_t now;
    struct tm *local;

    if (override != NULL && override[0] != '\0' && strcmp(override, "auto") != 0) {
        if (snprintf(path, pathSize, "%s", override) >= (int)pathSize) return 0;
    } else {
        if (!RagePlatformUserStateDirectory(directory, sizeof(directory)) ||
            !RagePlatformEnsureDirectory(directory)) return 0;
        if (snprintf(path, pathSize, "%s%srage-racer.log", directory,
#ifdef _WIN32
                     "\\"
#else
                     "/"
#endif
                     ) >= (int)pathSize) return 0;
    }
    file = freopen(path, "a", stderr);
    if (file == NULL) return 0;
    setvbuf(stderr, NULL, _IOLBF, 0);
    now = time(NULL);
    local = localtime(&now);
    fprintf(stderr, "\n=== Rage Racer session %04d-%02d-%02d %02d:%02d:%02d ===\n",
            local != NULL ? local->tm_year + 1900 : 0,
            local != NULL ? local->tm_mon + 1 : 0,
            local != NULL ? local->tm_mday : 0,
            local != NULL ? local->tm_hour : 0,
            local != NULL ? local->tm_min : 0,
            local != NULL ? local->tm_sec : 0);
    fprintf(stderr, "rage-port: diagnostic log=%s\n", path);
    return 1;
}
