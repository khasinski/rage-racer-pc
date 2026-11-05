#ifndef RAGE_HD_DOWNLOAD_H
#define RAGE_HD_DOWNLOAD_H
#include "hd_pack.h"
#include <stdint.h>
typedef int (*HdDownloadProgress)(void *context, const char *stage,
                                  uint64_t done, uint64_t total);
typedef struct HdRelease {
    char version[65], region[16], sha256[65], url[1200];
    uint64_t size;
} HdRelease;
int HdDownloadParseCatalog(const char *json, size_t size, const char *base,
                          const char *region, HdRelease *release);
/* HTTPS, or explicit loopback HTTP for local development. No redirects.
 * Returns 2 if the same release is already installed and verified, 1 after
 * installation, 0 on failure/cancellation. Previous active pack is retained. */
int HdDownloadInstall(const char *base, const char *store, const char *region,
                      HdDownloadProgress progress, void *context,
                      char *error, size_t capacity);
#endif
