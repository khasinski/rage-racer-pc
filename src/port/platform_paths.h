#ifndef RAGE_PLATFORM_PATHS_H
#define RAGE_PLATFORM_PATHS_H

#include <stddef.h>

/* Returns an existing user configuration file, then an existing file beside
 * the executable (or inside its macOS Resources directory), then the plain
 * name for developer builds run from the source tree. */
/* Path-producing functions clear a valid output buffer when they fail. */
int PlatformFindConfigFile(const char *argv0, const char *name, char *path,
                           size_t pathSize);
/* Directory holding the running executable, which is where a portable
 * install keeps the disc image alongside the game. */
int PlatformExecutableDirectory(const char *argv0, char *path,
                                size_t pathSize);
/* Returns the portable state root only when it already contains `bu00`.
 * This preserves saves from older unpacked releases without creating new
 * files beside the executable for ordinary installs. */
int PlatformExistingPortableStateDirectory(
    const char *executableDirectory, char *path, size_t pathSize);
int PlatformUserConfigDirectory(char *path, size_t pathSize);
int PlatformUserStateDirectory(char *path, size_t pathSize);
int PlatformUserConfigPath(const char *name, char *path, size_t pathSize);
int PlatformEnsureDirectory(const char *path);

#endif
