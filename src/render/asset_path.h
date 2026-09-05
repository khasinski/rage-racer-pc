#ifndef RAGE_ASSET_PATH_H
#define RAGE_ASSET_PATH_H
#include <stddef.h>

/* Lexical relative file path, counted bytes (no terminating NUL required).
 * Does not resolve symlinks or provide a filesystem sandbox. */
int AssetPathIsRelativeFile(const char *path, size_t size);
#endif
