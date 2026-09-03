#ifndef RAGE_DISC_DISCOVERY_H
#define RAGE_DISC_DISCOVERY_H

#include <stddef.h>

typedef int (*DiscImageValidator)(void *context, const char *path);

int DiscPathIsCue(const char *path);
int DiscPathIsChd(const char *path);
int DiscPathIsBin(const char *path);
int DiscPathIsSupportedImage(const char *path);
int DiscReadSavedPath(const char *configPath, char *path, size_t pathSize);
int DiscPathsReferToSameFile(const char *first, const char *second);

/* Tries supported files in directory until validate accepts one. */
int DiscDiscoverImage(const char *directory, char *path, size_t pathSize,
                      DiscImageValidator validate, void *context);

#endif
