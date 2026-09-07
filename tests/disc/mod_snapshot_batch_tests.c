#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "render/mod_file_snapshot.h"

static unsigned copies;
static int changeEarlierSource;
static const char *firstSource = "snapshot_batch_first.tmp";

/* Compile-time interception only: production has no test hook or timing
 * dependency. Both copies still execute the real file-copy implementation. */
static int CopyAndChangeEarlierSource(const char *source, const char *target,
                                      size_t *total) {
    int ok = ModFileSnapshotCopy(source, target, total);
    if (ok && ++copies == 2 && changeEarlierSource) {
        FILE *file = fopen(firstSource, "r+b");
        assert(file);
        assert(fputc('X', file) == 'X');
        assert(fclose(file) == 0);
    }
    return ok;
}

#define ModFileSnapshotCopy CopyAndChangeEarlierSource
#include "../../launcher/native/mod_snapshot.h"
#undef ModFileSnapshotCopy

int main(void) {
    const char *secondSource = "snapshot_batch_second.tmp";
    const char *firstTarget = "snapshot_batch_first_copy.tmp";
    const char *secondTarget = "snapshot_batch_second_copy.tmp";
    const char *requests[2] = {"snapshot_batch_request0.tmp", "snapshot_batch_request1.tmp"};
    const char *json = "[[\"snapshot_batch_first.tmp\",\"snapshot_batch_first_copy.tmp\"],"
                       "[\"snapshot_batch_second.tmp\",\"snapshot_batch_second_copy.tmp\"]]";
    for (int changed = 0; changed < 2; ++changed) {
        const char *request = requests[changed];
        assert(ModFileWriteExclusive(firstSource, "first", 5));
        assert(ModFileWriteExclusive(secondSource, "second", 6));
        assert(ModFileWriteExclusive(request, json, strlen(json)));
        assert(freopen(request, "rb", stdin));
        copies = 0;
        changeEarlierSource = changed;
        assert(SnapshotCommand() == changed);
        assert(copies == 2);
        assert(ModFileSnapshotMatches(secondSource, secondTarget));
        assert(ModFileSnapshotMatches(firstSource, firstTarget) == !changed);
        /* The command does not own staging cleanup; its caller does. */
        assert(remove(firstTarget) == 0);
        assert(remove(secondTarget) == 0);
        assert(remove(firstSource) == 0);
        assert(remove(secondSource) == 0);
    }
    assert(fclose(stdin) == 0);
    assert(remove(requests[0]) == 0);
    assert(remove(requests[1]) == 0);
    return 0;
}
