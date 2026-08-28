#include "diagnostic_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "runtime_config.h"
#include "platform_paths.h"

int DiagnosticLogOpen(char *path, size_t pathSize) {
    const char *override = RuntimeConfigGetForced("diagnostics.log");
    char directory[1024];
    FILE *file;
    time_t now;
    struct tm *local;

    if (override != NULL && override[0] != '\0' && strcmp(override, "auto") != 0) {
        if (snprintf(path, pathSize, "%s", override) >= (int)pathSize) return 0;
    } else {
        if (!PlatformUserStateDirectory(directory, sizeof(directory)) ||
            !PlatformEnsureDirectory(directory)) return 0;
        if (snprintf(path, pathSize, "%s%srage-racer.log", directory,
#ifdef _WIN32
                     "\\"
#else
                     "/"
#endif
                     ) >= (int)pathSize) return 0;
    }
    /* Say where the diagnostics went while stderr still reaches the terminal.
     * This call takes stderr away, so without the notice here the only record
     * of the path lands in the file nobody has found yet, and redirecting the
     * command's stderr captures an empty stream. */
    fprintf(stderr, "rage-port: diagnostics go to %s\n", path);
    fflush(stderr);
    file = freopen(path, "a", stderr);
    if (file == NULL) return 0;
    /* Microsoft's CRT accepts _IOLBF but buffers a whole block anyway, so a
     * crash takes the diagnostics down with it and leaves an empty file.
     * Write straight through there; the log exists for the runs that die. */
#ifdef _WIN32
    setvbuf(stderr, NULL, _IONBF, 0);
#else
    setvbuf(stderr, NULL, _IOLBF, 0);
#endif
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
