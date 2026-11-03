#ifndef RAGE_HD_PACK_H
#define RAGE_HD_PACK_H
#include <stddef.h>
#include "render/mod_manifest.h"
typedef int (*HdPackProgress)(void *context,int percent);
typedef struct HdPackState {
    char directory[1400], hash[65], inventoryHash[65];
    char name[201],version[65],region[16];
    int enabled;
} HdPackState;
/* Local .rrhd archive: bounded data-only records, each SHA-256 verified.
 * Install publishes active state only after all files and references validate.
 * Callback returning zero cancels; failed installs preserve the old pointer. */
int HdPackCreate(const char *source,const char *output,char *error,size_t capacity);
int HdPackInstall(const char *archive,const char *store,const char *region,
                 HdPackProgress progress,void *context,char *error,size_t capacity);
int HdPackReadState(const char *store,const char *region,HdPackState *state);
int HdPackSetEnabled(const char *store,const HdPackState *state,int enabled);
int HdPackVerify(const char *store,const HdPackState *state,
                 HdPackProgress progress,void *context,char *error,size_t capacity);
RageModManifest *HdPackReadManifest(const char *directory);
#endif
