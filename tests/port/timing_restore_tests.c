#include <stdio.h>
#include <math.h>

#include "timing_control.h"

static int failures;

static void Expect(const char *what, int got, int want) {
    if (got != want) {
        printf("%s: expected %d, got %d\n", what, want, got);
        failures++;
    }
}

int main(void) {
    /* PAL wants 20000us a frame, NTSC 16683. The platform resets itself to the
     * NTSC figure when it creates its video device, which is what has to be
     * noticed and put back. */
    Expect("PAL left at NTSC", TimingNeedsRestore(16683.0, 50), 1);
    Expect("PAL already right", TimingNeedsRestore(20000.0, 50), 0);
    Expect("NTSC already right", TimingNeedsRestore(16666.7, 60), 0);
    Expect("NTSC left at PAL", TimingNeedsRestore(20000.0, 60), 1);

    /* Nothing measured yet is not drift. */
    Expect("no measurement", TimingNeedsRestore(0.0, 50), 0);
    Expect("negative measurement", TimingNeedsRestore(-5.0, 50), 0);
    Expect("NaN measurement", TimingNeedsRestore(NAN, 50), 0);
    Expect("infinite measurement", TimingNeedsRestore(INFINITY, 50), 0);
    Expect("zero base frequency", TimingNeedsRestore(20000.0, 0), 0);
    Expect("negative base frequency", TimingNeedsRestore(20000.0, -50), 0);

    /* Rounding in the platform's own bookkeeping must not cause a restore
     * every frame, which would keep resetting its drift compensation. */
    Expect("PAL within a microsecond", TimingNeedsRestore(20000.4, 50), 0);
    Expect("PAL a microsecond under", TimingNeedsRestore(19999.5, 50), 0);
    Expect("PAL two microseconds out", TimingNeedsRestore(20002.0, 50), 1);

    if (failures) {
        printf("%d timing restore assertion(s) failed\n", failures);
        return 1;
    }
    printf("timing restore notices the platform's NTSC reset and nothing else\n");
    return 0;
}
