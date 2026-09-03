#include <math.h>

/* Whether the platform's frame time has drifted off the standard the game was
 * started in. It resets itself to the NTSC figure when it creates its video
 * device, which happens after the standard is chosen and again on display mode
 * changes, so this is asked every frame. The one microsecond of slack keeps
 * the platform's own rounding from triggering a restore, which would reset its
 * drift compensation continuously. */
int TimingNeedsRestore(double currentFrameTimeUs, int baseHz) {
    double wanted;

    if (!isfinite(currentFrameTimeUs) || currentFrameTimeUs <= 0.0 ||
        baseHz <= 0) {
        return 0;
    }
    wanted = 1000000.0 / (double)baseHz;
    return currentFrameTimeUs > wanted + 1.0 || currentFrameTimeUs < wanted - 1.0;
}
