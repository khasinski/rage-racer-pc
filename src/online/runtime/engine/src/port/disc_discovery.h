#ifndef RAGE_DISC_DISCOVERY_H
#define RAGE_DISC_DISCOVERY_H

#include <stddef.h>

typedef int (*DiscImageValidator)(void *context, const char *path);

int DiscPathIsCue(const char *path);
int DiscPathIsBin(const char *path);

#endif
