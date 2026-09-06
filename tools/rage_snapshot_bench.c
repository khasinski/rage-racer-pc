/* Standalone CPU-cost probe; not a game frame-time or GPU benchmark.
 * Build with render_world_snapshot.c, render_world.c, -Isrc and -lm. */
#include "render/render_world_snapshot.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static int Measure(unsigned count, unsigned iterations) {
    RageRenderMeshInstance *instances = calloc(count, sizeof(*instances));
    if (!instances) return 0;
    for (unsigned i = 0; i < count; ++i) instances[i].assetKey = i;
    RageRenderWorld world = {0};
    world.instances = instances;
    world.instanceCount = world.instanceCapacity = count;
    RageRenderWorldSnapshot snapshot = {0};
    int ok = RenderWorldSnapshotCopy(&snapshot, &world);
    RageRenderMeshInstance *allocation = snapshot.instances;
    clock_t begin = clock();
    for (unsigned i = 0; ok && i < iterations; ++i) {
        world.frame = i;
        instances[count - 1].assetKey = i;
        ok = RenderWorldSnapshotCopy(&snapshot, &world) &&
             snapshot.instances == allocation &&
             snapshot.world.frame == i &&
             snapshot.instances[count - 1].assetKey == i;
    }
    clock_t end = clock();
    if (begin == (clock_t)-1 || end == (clock_t)-1 || end <= begin) ok = 0;
    if (ok) printf("instances=%u bytes=%zu iterations=%u cpu_us_per_copy=%.3f allocation_reused=1\n",
        count, (size_t)count * sizeof(*instances), iterations,
        1e6 * (double)(end - begin) / CLOCKS_PER_SEC / iterations);
    RenderWorldSnapshotRelease(&snapshot);
    free(instances);
    return ok;
}

int main(void) {
    const unsigned counts[] = {256, 2048, 8192};
    for (unsigned i = 0; i < sizeof(counts) / sizeof(counts[0]); ++i)
        if (!Measure(counts[i], 20000)) return 1;
    return 0;
}
