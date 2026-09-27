#ifndef RAGE_RUNTIME_PARSE_H
#define RAGE_RUNTIME_PARSE_H

/* Parse a complete integer string using the requested strtol base. */
int RuntimeParseInt(const char *text, int base, int minimum, int maximum,
                    int *result);

#endif
