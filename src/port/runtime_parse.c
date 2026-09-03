#include "runtime_parse.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

int RuntimeParseInt(const char *text, int base, int minimum, int maximum,
                    int *result) {
    char *end;
    long value;

    if (text == NULL || text[0] == '\0' || result == NULL ||
        minimum > maximum) {
        return 0;
    }

    errno = 0;
    value = strtol(text, &end, base);
    if (errno == ERANGE || end == text || *end != '\0' ||
        value < minimum || value > maximum ||
        value < INT_MIN || value > INT_MAX) {
        return 0;
    }

    *result = (int)value;
    return 1;
}
