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
    puts("native snapshot byte equality, exclusive creation and cleanup passed");
    return 0;
}
