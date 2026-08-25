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
int RageModernInit(const RagePortConfig *config);

void RageModernShutdown(void);

/* 1 when the modern renderer is initialized and selected for presentation.
 * Game logic must never branch on this; it gates presentation-side work
 * only. */
int RageModernIsEnabled(void);

/* Switches presentation without restarting or changing game state. The
 * compatibility framebuffer keeps rendering in both modes. */
void RageModernToggle(void);

/* Frame-sync wait hook: presents interpolated frames between logic ticks
 * when an FPS mode is configured. No-op otherwise. */
void RageModernFrameWaitTick(int frameLimit);
void RageModernLogicFrameReady(uint32_t frame);

#endif
