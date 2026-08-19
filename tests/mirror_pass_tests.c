#include <stdio.h>

#include "../src/port/include/mirror_pass.h"

static int failures;

static void Expect(const char *what, int got, int want) {
    if (got != want) {
        printf("%s: expected %d, got %d\n", what, want, got);
        failures++;
    }
}

int main(void) {
    /* SCRATCH_MIRROR is the combined flag; the rear-view pass is where it
     * differs from the course setting. */
    Expect("normal course, main pass", RageMirrorRearViewPass(0, 0), 0);
    Expect("normal course, rear view", RageMirrorRearViewPass(1, 0), 1);
    Expect("mirrored course, main pass", RageMirrorRearViewPass(1, 1), 0);
    Expect("mirrored course, rear view", RageMirrorRearViewPass(0, 1), 1);

    /* The dispatcher reflects the rear view's own matrix, and only when the
     * course has not reflected the camera already. */
    Expect("main pass never reflects there", RageMirrorDispatcherReflects(0, 0), 0);
    Expect("rear view of a normal course", RageMirrorDispatcherReflects(1, 0), 1);
    Expect("mirrored course leaves it alone", RageMirrorDispatcherReflects(1, 1), 0);
    Expect("rear view of a mirrored course cancels out",
           RageMirrorDispatcherReflects(0, 1), 0);

    /* The property that matters: every pass ends up reflected exactly as often
     * as SCRATCH_MIRROR says, counting the camera for the main pass and the
     * dispatcher for the rear view, which uses a matrix of its own. */
    {
        int course, scratch;
        for (course = 0; course <= 1; course++) {
            for (scratch = 0; scratch <= 1; scratch++) {
                int rearView = RageMirrorRearViewPass(scratch, course);
                int applied = rearView ? RageMirrorDispatcherReflects(scratch, course)
                                       : course;
                if (applied != scratch) {
                    printf("course=%d scratch=%d: %d reflections applied, wanted %d\n",
                           course, scratch, applied, scratch);
                    failures++;
                }
            }
        }
    }

    if (failures) {
        printf("%d mirror pass assertion(s) failed\n", failures);
        return 1;
    }
    printf("mirror pass bookkeeping balances for every combination\n");
    return 0;
}
