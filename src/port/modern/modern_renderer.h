#ifndef RAGE_MODERN_RENDERER_H
#define RAGE_MODERN_RENDERER_H

#include <stdint.h>

/* The modern (enhanced) renderer. The compat path always runs and remains
 * the behavioural oracle; this module only changes what is presented.
 * See docs/modern_renderer_plan.md. */

#include "../port_config.h"

/* Registers the PsyZ device/present hooks when the configuration selects
 * the modern renderer. Safe to call before platform initialization.
 * Returns 1 on success (including when the modern renderer is disabled). */
int ModernInit(const RagePortConfig *config);

/* Release presentation resources before session assets and restore host hooks.
 * Safe after failed initialization and on repeated calls. Device reset uses
 * the separate backend destroy hook and does not end the asset session. */
void ModernShutdown(void);

/* Recreate presentation resources/hooks while retaining the asset session.
 * Call between presentations on the render thread. This does not reset game
 * simulation, disc state or a completed asset session. Returns 0 on failure. */
int ModernRestartPresentation(const RagePortConfig *config);


/* 1 when the modern renderer is initialized and selected for presentation.
 * Game logic must never branch on this; it gates presentation-side work
 * only. */
int ModernIsEnabled(void);

/* Switches presentation without restarting or changing game state. The
 * compatibility framebuffer keeps rendering in both modes. */
void ModernToggle(void);

/* Frame-sync wait hook: presents interpolated frames between logic ticks
 * when an FPS mode is configured. No-op otherwise. */
void ModernFrameWaitTick(int frameLimit);
void ModernLogicFrameReady(uint32_t frame);
/* Called after DrawSync and the complete track texture swap, before waiting
 * for presentation. Never capture a new frame from inside scene construction. */
void ModernFrameTexturesReady(void);

#endif
