#include "render/mod_file_snapshot.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <sys/stat.h>
#include <unistd.h>
#endif

static void RejectSource(const char *source, const char *target) {
    size_t total = 17;
    assert(!ModFileSnapshotCopy(source, target, &total));
    assert(total == 17);
    FILE *file = fopen(target, "rb");
    assert(!file);
}

static void UnicodePaths(void) {
    const char *source = "mod_snapshot_\xc4\x85_source.tmp";
    const char *target = "mod_snapshot_\xc5\xbc_target.tmp";
#ifdef _WIN32
    FILE *file = _wfopen(L"mod_snapshot_\x0105_source.tmp", L"wbx");
#else
    FILE *file = fopen(source, "wbx");
#endif
    assert(file);
    assert(fputs("unicode snapshot", file) >= 0);
    assert(fclose(file) == 0);
    size_t total = 0;
    assert(ModFileSnapshotCopy(source, target, &total));
    assert(total == strlen("unicode snapshot"));
    assert(!ModFileSnapshotCopy(source, target, &total));
#ifdef _WIN32
    file = _wfopen(L"mod_snapshot_\x017c_target.tmp", L"rb");
#else
    file = fopen(target, "rb");
#endif
    assert(file);
    char bytes[17] = {0};
    assert(fread(bytes, 1, sizeof(bytes), file) == strlen("unicode snapshot"));
    assert(!strcmp(bytes, "unicode snapshot"));
    assert(fclose(file) == 0);
#ifdef _WIN32
    assert(_wremove(L"mod_snapshot_\x017c_target.tmp") == 0);
#else
    assert(remove(target) == 0);
#endif
    total = 1024u * 1024u * 1024u;
    assert(!ModFileSnapshotCopy(source, target, &total));
#ifdef _WIN32
    file = _wfopen(L"mod_snapshot_\x017c_target.tmp", L"rb");
    assert(_wremove(L"mod_snapshot_\x0105_source.tmp") == 0);
#else
    file = fopen(target, "rb");
    assert(remove(source) == 0);
#endif
    assert(!file); /* Failed copy cleans up its Unicode output path too. */
}

int main(void) {
    const char *source = "mod_snapshot_source.tmp", *target = "mod_snapshot_target.tmp";
    unsigned char bytes[70000], copied[70000];
    size_t total = 0;
    for (size_t i=0;i<sizeof(bytes);++i) bytes[i]=(unsigned char)(i%251);
    FILE *file = fopen(source,"wbx"); assert(file);
    assert(fwrite(bytes,1,sizeof(bytes),file) == sizeof(bytes)); assert(fclose(file) == 0);
    assert(ModFileSnapshotCopy(source,target,&total)); assert(total == sizeof(bytes));
    assert(!ModFileSnapshotCopy(source,target,&total)); /* Never overwrite. */
    file = fopen(target,"rb"); assert(file);
    assert(fread(copied,1,sizeof(copied),file) == sizeof(copied)); assert(fclose(file) == 0);
    assert(!memcmp(bytes,copied,sizeof(bytes)));
    assert(!ModFileSnapshotCopy(source,source,&total));
    assert(!ModFileSnapshotCopy(NULL,target,&total));
    assert(!ModFileSnapshotCopy(source,NULL,&total));
    assert(!ModFileSnapshotCopy(source,target,NULL));
    assert(remove(target) == 0);
    RejectSource(".", target);
#ifndef _WIN32
    const char *link = "mod_snapshot_link.tmp";
    assert(symlink(source, link) == 0);
    RejectSource(link, target);
    assert(unlink(link) == 0);
    assert(symlink("mod_snapshot_missing.tmp", link) == 0);
    RejectSource(link, target);
    assert(unlink(link) == 0);
    assert(mkfifo(link, 0600) == 0);
    RejectSource(link, target); /* Must reject without waiting for a writer. */
    assert(unlink(link) == 0);
#endif
    total = 1024u*1024u*1024u;
    assert(!ModFileSnapshotCopy(source,target,&total));
    file = fopen(target,"rb"); assert(!file); /* Failed copy removed its own output. */
    assert(remove(source) == 0);
    UnicodePaths();
    puts("native snapshot byte equality, exclusive creation and cleanup passed");
    return 0;
}
