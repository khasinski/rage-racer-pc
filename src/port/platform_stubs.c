#include "rage/render_world_game.h"
#include "modern/modern_renderer.h"
#include "modern/scene_capture.h"
#include "fmv_audio.h"
#include "timing_control.h"
#include <libgte.h>

extern int32_t g_FrameCounter;

/*
 * Fallbacks for the two hooks the smoke and scenario builds provide. They are
 * weak so a build that carries the real one wins, which every build of the
 * game does. The Windows linker does not honour weak definitions, and takes
 * two of the same symbol as an error rather than a preference, so there it
 * gets the declarations only and the real definitions do the work.
 */
void PortSmokeBeforeSceneHandler(void);
void PortScenarioBeforeSceneHandler(void);
#ifndef _MSC_VER
void PortSmokeBeforeSceneHandler(void) __attribute__((weak));
void PortSmokeBeforeSceneHandler(void) {}
void PortScenarioBeforeSceneHandler(void) __attribute__((weak));
void PortScenarioBeforeSceneHandler(void) {}
#else
/* The scenario hook is in every build of the game, so it needs no stand-in
 * here; the smoke hook is only in the smoke build, so it does, except in the
 * smoke build itself. */
#ifndef RAGE_SMOKE_TARGET
void PortSmokeBeforeSceneHandler(void) {}
#endif
#endif

void PortBeforeSceneHandler(void) {
    HostFmvAudioTick();
    TimingApply();
#ifdef RAGE_SMOKE_TARGET
    PortSmokeBeforeSceneHandler();
    /* Scenario input is synthesized after physical/test input sampling so it
     * cannot be cleared by the pad edge update in the same frame. */
    PortScenarioBeforeSceneHandler();
#else
    PortScenarioBeforeSceneHandler();
    PortSmokeBeforeSceneHandler();
#endif
    GameRenderWorldBeginFrame((uint64_t)g_FrameCounter);
    CaptureFrameBegin();
}

void PortAfterSceneHandler(void) {
    GameRenderWorldPublishCurrentCamera();
    if (ModernIsEnabled()) {
        GameRenderWorldPublishCourseObjects();
        GameRenderWorldPublishTerrainGrid();
        GameRenderWorldPublishRaceCars();
        GameRenderWorldDiscardLegacyMirror();
    }
    CaptureFrameEnd();
    ModernLogicFrameReady((uint32_t)g_FrameCounter);
}

void PortDuringFrameWait(int frameLimit) {
    ModernFrameWaitTick(frameLimit);
}

long SpuTransferStatus(void *address, long mode) {
    (void)address;
    (void)mode;
    return 0;
}
