#ifndef CONTENT_ACTIVE_BUILD_H
#define CONTENT_ACTIVE_BUILD_H
#include <stddef.h>
typedef struct ActiveBuild {
    char directory[1024];
    char profile[1024];
} ActiveBuild;
/* 1: valid immutable build; 0: no pointer; -1: rejected. Output is unchanged
 * on failure. Source is SHA-256 of original RAGE.BIN, with sha256: prefix. */
int ActiveBuildOpen(const char *root,const char *region,const char *source,
                    ActiveBuild *build,char *error,size_t errorSize);
#endif
