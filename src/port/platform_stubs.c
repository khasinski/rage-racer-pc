#include "rage/render_world_game.h"
#include "modern/modern_renderer.h"
#include "modern/scene_capture.h"
#include "fmv_audio.h"
#include "mod_assets.h"
#include "timing_control.h"
#include "game/asset.h"
#include "game/audio.h"
#include "game/race.h"
#include "game/state.h"
#include "graphics_overlay.h"
#include <libetc.h>
#include <libgte.h>

extern int32_t g_FrameCounter;
static unsigned s_extraCourseGeneration;

static void SyncExtraCourseCatalog(void) {
    unsigned generation = ModAssetsGeneration();
    u32 mask = 0;
    s32 i;

    if (s_extraCourseGeneration == generation) return;
    s_extraCourseGeneration = generation;
    for (i = 0; i < EXTRA_COURSE_SLOT_COUNT; i++) {
        s32 slot = EXTRA_COURSE_SLOT_BASE + i;
        s32 firstAsset = TrackCourseAssetIndex(ASSET_TRACK_1ST_BASE, 0, slot);
        s32 secondAsset = TrackCourseAssetIndex(ASSET_TRACK_2ND_BASE, 0, slot);
        if (firstAsset >= 0 && secondAsset >= 0 &&
            ModAssetExists(firstAsset) && ModAssetExists(secondAsset)) {
            mask |= (1u << (u32)i);
        }
        ModAssetCourseLabel(i,
                            g_RaceSession.extraCourseNames[i],
                            sizeof(g_RaceSession.extraCourseNames[i]),
                            g_RaceSession.extraCourseLengths[i],
                            sizeof(g_RaceSession.extraCourseLengths[i]));
    }
    g_RaceSession.extraCourseMask = mask;
}

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
    SyncExtraCourseCatalog();
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
    GameRenderWorldEndFrame();
    ModernLogicFrameReady((uint32_t)g_FrameCounter);
}

void PortDuringFrameWait(int frameLimit) {
    ModernFrameWaitTick(frameLimit);
}

void PortAfterFrameTransfers(void) {
    ModernFrameTexturesReady();
}

long SpuTransferStatus(void *address, long mode) {
    (void)address;
    (void)mode;
    return 0;
}

void PortHoldWhilePaused(void) {
    int muted = 0;

    /* Lays out the on-screen pad when there is one; a no-op otherwise. */
    PortGraphicsOverlayTick();
    if (!PortGraphicsOverlayActive()) return;
    /* Music is left running; the engine and effect voices would otherwise
     * drone on over a frozen race. A race pause has already silenced them. */
    if (g_RacePaused == 0) {
        ForceAllEffectVoicesEnabled(0);
        muted = 1;
    }
    while (PortGraphicsOverlayActive()) {
        PortGraphicsOverlayTick();
        VSync(0);
    }
    PortGraphicsOverlayTick();
    if (muted) ForceAllEffectVoicesEnabled(1);
    if (PortGraphicsOverlayTakeClosed()) {
        /* Keys used inside the panel are latched for the pad. Read them out
         * now so the game sees neither a stray press nor a stale one. */
        UpdatePadState();
        UpdatePadState();
    }
    /* VSync counts the time spent here as one very long frame. */
    VSync(0);
}
