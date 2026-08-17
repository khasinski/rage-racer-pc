#ifndef RAGE_PLATFORM_PATHS_H
#define RAGE_PLATFORM_PATHS_H

#include <stddef.h>

/* Returns an existing user configuration file, then an existing file beside
 * the executable (or inside its macOS Resources directory), then the plain
 * name for developer builds run from the source tree. */
int RagePlatformFindConfigFile(const char *argv0, const char *name,
                               char *path, size_t pathSize);
/* Directory holding the running executable, which is where a portable
 * install keeps the disc image alongside the game. */
int RagePlatformExecutableDirectory(const char *argv0, char *path,
                                    size_t pathSize);
int RagePlatformUserConfigDirectory(char *path, size_t pathSize);
int RagePlatformUserStateDirectory(char *path, size_t pathSize);
int RagePlatformUserConfigPath(const char *name, char *path, size_t pathSize);
int RagePlatformTemporaryDirectory(char *path, size_t pathSize);
int RagePlatformEnsureDirectory(const char *path);

#endif
