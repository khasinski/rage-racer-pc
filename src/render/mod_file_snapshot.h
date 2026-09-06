#ifndef RAGE_MOD_FILE_SNAPSHOT_H
#define RAGE_MOD_FILE_SNAPSHOT_H
#include <stddef.h>
/* Destination must not exist. Copies and verifies through the same source
 * handle. Caller owns staging cleanup; total enforces a 1 GiB batch budget.
 * Individual files are limited to 128 MiB. Rejects nonregular sources and
 * final-component symlinks/reparse points on the opened handle. Windows paths
 * are UTF-8. Parent directories are not pinned. This is not an adversarial
 * filesystem sandbox or a multi-file point-in-time transaction. */
int ModFileSnapshotCopy(const char *source, const char *target, size_t *total);
#endif
