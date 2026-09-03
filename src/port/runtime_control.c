#include <stdio.h>
#include <limits.h>

#include "modern/scene_capture.h"
#include "runtime_config.h"
#include "scenario_control.h"

extern int g_SceneId;
extern int g_SceneTimer;

/*
 * The release executable stops when a scenario asks it to. It also honours the
 * capture stop the smoke executable has, because the modern renderer only runs
 * where there is a window, so a picture of what it draws can only be taken
 * here.
 */
int PortShouldExit(int frame_number) {
    static long stopScene = -1, stopTimer = -1;
    static const char *capturePath;
    static int resolved;

    if (!resolved) {
        resolved = 1;
        capturePath = RuntimeConfigGet("capture.path");
        stopScene = RuntimeConfigInt("stop.scene", -1, 0, INT_MAX);
        stopTimer = RuntimeConfigInt("stop.timer", -1, 0, INT_MAX);
    }
    (void)frame_number;

    if (stopScene >= 0 && stopTimer >= 0 && g_SceneId == (int)stopScene &&
        g_SceneTimer >= (int)stopTimer) {
        if (capturePath != NULL && capturePath[0] != '\0') {
            if (ModernCaptureFrame(capturePath))
                fprintf(stderr, "rage-port: modern frame written to %s\n",
                        capturePath);
            else
                fprintf(stderr,
                        "rage-port: cannot capture %s; the modern renderer is not running\n",
                        capturePath);
        }
        fprintf(stderr, "rage-port: stopping at scene %d timer %d\n",
                g_SceneId, g_SceneTimer);
        return 1;
    }
    return PortScenarioShouldExit();
}
