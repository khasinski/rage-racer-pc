#include "common.h"
#include <stdio.h>
#include <string.h>
#include "psyq/cd.h"
#include "psyq/cd_internal.h"


CdlFILE *DsSearchFile(CdlFILE *out, char *path) {
    char buf[32];
    char *p;
    char *b;
    long type;
    long n;
    long i;
    CdlFILE *rec;
    char *nm;

    if (g_CdCachedShellOpenCount != g_CdShellOpenCount) {
        if (!CD_newmedia()) {
            return 0;
        }
        g_CdCachedShellOpenCount = g_CdShellOpenCount;
    }
    if (*path != '\\') {
        return 0;
    }
    type = 1;
    buf[0] = 0;
    p = path;
    n = 0;
    while (n < 8) {
        b = buf;
        while (*p != '\\') {
            if (*p == 0) {
                break;
            }
            *b++ = *p++;
        }
        if (*p == 0) {
            break;
        }
        p++;
        *b = 0;
        type = DS_searchdir(type, buf);
        if (type == -1) {
            buf[0] = 0;
            break;
        }
        n++;
    }
    if (n >= 8) {
        if (g_CdDebugLevel > 0) {
            printf(g_FmtCdPathLevelError, path, n);
        }
        return 0;
    }
    if (buf[0] == 0) {
        if (g_CdDebugLevel > 0) {
            printf(g_FmtCdDirNotFound, path);
        }
        return 0;
    }
    *b = 0;
    if (CD_cachefile(type) == 0) {
        if (g_CdDebugLevel > 0) {
            printf(g_MsgCdDiscError);
        }
        return 0;
    }
    if (g_CdDebugLevel >= 2) {
        printf(g_FmtCdSearching, buf);
    }
    {
        char *base = g_CdFileCache[0].name;
        rec = (CdlFILE *)(base - 8);
        nm = base;
    }
    for (i = 0; i < 64; i++) {
        if (g_CdFileCache[i].name[0] == 0) {
            break;
        }
        if (CD_namecmp(nm, buf)) {
            if (g_CdDebugLevel >= 2) {
                printf(g_FmtCdFileFound, buf);
            }
            *out = *rec;
            return rec;
        }
        rec++;
        nm += 24;
    }
    if (g_CdDebugLevel > 0) {
        printf(g_FmtCdFileNotFound, buf);
    }
    return 0;
}
