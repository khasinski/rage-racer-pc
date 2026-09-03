#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#include "runtime_config.h"

/*
 * Diagnostics boundary for the recovered game code. A diagnostic key is just a
 * setting under `diagnostics.`, so this is only the prefix.
 *
 * It used to carry a table naming an environment variable for each of the
 * twenty-one keys it knew, which meant a new trace had to be registered in two
 * places before it could be switched on, and an unregistered key silently
 * answered "off". runtime_config.c now derives the environment name from the
 * key and keeps the historical exceptions in one table, so neither is needed.
 */
static const char *FullKey(const char *key, char *buffer, size_t size) {
    int written;

    if (key == NULL || key[0] == '\0' || buffer == NULL || size == 0) {
        return NULL;
    }
    written = snprintf(buffer, size, "diagnostics.%s", key);
    if (written < 0 || (size_t)written >= size) return NULL;
    return buffer;
}

const char *DiagnosticsValue(const char *key) {
    char full[128];
    return FullKey(key, full, sizeof(full)) ? RuntimeConfigGet(full) : NULL;
}

int DiagnosticsEnabled(const char *key) {
    char full[128];
    return FullKey(key, full, sizeof(full)) ? RuntimeConfigEnabled(full) : 0;
}

int DiagnosticsIntValue(const char *key, int fallback) {
    char full[128];

    return FullKey(key, full, sizeof(full))
               ? RuntimeConfigInt(full, fallback, INT_MIN, INT_MAX)
               : fallback;
}

void Trace(const char *topic, const char *format, ...) {
    va_list arguments;
    fputs(topic, stderr);
    fputc(' ', stderr);
    va_start(arguments, format);
    vfprintf(stderr, format, arguments);
    va_end(arguments);
    fputc('\n', stderr);
}
