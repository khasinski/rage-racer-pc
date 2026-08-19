#include <stdio.h>
#include <stdlib.h>

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
int RagePortShouldExit(int frame_number) {
    static long stopScene = -1, stopTimer = -1;
    static const char *capturePath;
    static int resolved;

    if (!resolved) {
        const char *scene = RageRuntimeConfigGet("stop.scene");
        const char *timer = RageRuntimeConfigGet("stop.timer");
        resolved = 1;
        capturePath = RageRuntimeConfigGetLegacy("capture.path",
                                                 "RAGE_PORT_CAPTURE_PATH");
        if (scene != NULL) stopScene = strtol(scene, NULL, 10);
        if (timer != NULL) stopTimer = strtol(timer, NULL, 10);
    }
    (void)frame_number;

    if (stopScene >= 0 && stopTimer >= 0 && g_SceneId == (int)stopScene &&
        g_SceneTimer >= (int)stopTimer) {
        if (capturePath != NULL && capturePath[0] != '\0') {
            if (RageModernCaptureFrame(capturePath))
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
    return RagePortScenarioShouldExit();
}
