#include "mod_file_snapshot.h"
#include <stdio.h>
#include <string.h>

/* Copy through one open source handle, then compare the copied bytes against
 * a second read of that handle. Differing reads reject the copy; writers that
 * restore bytes between reads are not detected. Private staging
 * is validated after copying, never against the external source directory. */
int ModFileSnapshotCopy(const char *source, const char *target, size_t *total) {
    unsigned char buffer[65536], copy[65536];
    if (!source || !target || !total || *total > 1024u*1024u*1024u) return 0;
    FILE *input = fopen(source,"rb"), *output = NULL;
    size_t size = 0, n;
    int ok = 0;
    if (!input) return 0;
    output = fopen(target,"w+bx");
    if (!output) { fclose(input); return 0; }
    while ((n = fread(buffer,1,sizeof(buffer),input)) != 0) {
        if (size > 128u*1024u*1024u-n || *total > 1024u*1024u*1024u-n ||
            fwrite(buffer,1,n,output) != n) goto done;
        size += n; *total += n;
    }
    if (ferror(input) || fflush(output) || fseek(input,0,SEEK_SET) || fseek(output,0,SEEK_SET)) goto done;
    for (;;) {
        n = fread(buffer,1,sizeof(buffer),input);
        size_t copied = fread(copy,1,sizeof(copy),output);
        if (n != copied || memcmp(buffer,copy,n)) goto done;
        if (!n) break;
    }
    ok = !ferror(input) && !ferror(output);
done:
    if (fclose(input)) ok = 0;
    if (fclose(output)) ok = 0;
    if (!ok) remove(target);
    return ok;
}
