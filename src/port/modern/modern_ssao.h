#ifndef RAGE_MODERN_SSAO_H
#define RAGE_MODERN_SSAO_H

#include <SDL3/SDL.h>

/* Screen-space ambient occlusion for the native 3D scene. It reads the scene
 * depth buffer, builds an occlusion term, blurs it across surfaces only, and
 * multiplies it onto the colour target before any 2D drawing. */

typedef struct ModernSsaoView {
    /* x,y: view-space to clip scale; z,w: perspective depth terms, so that
     * ndcDepth = z + w / viewDepth. The same values the scene vertex shader
     * uses. */
    float projection[4];
} ModernSsaoView;

/* `fullscreenVertex` is the renderer's single-triangle vertex shader; it is
 * only read while the pipelines are built. Creates the pipelines and the two R8 occlusion targets for a scene target of
 * the given size. Returns 0 (with nothing allocated) on failure. */
int ModernSsaoCreate(SDL_GPUDevice *device, SDL_GPUShader *fullscreenVertex,
                    int width, int height);
void ModernSsaoDestroy(void);
int ModernSsaoReady(void);

/* Darkens `sceneColor` where `sceneDepth` shows creases and contact. The
 * depth texture must have been created with sampler usage. */
void ModernSsaoApply(SDL_GPUCommandBuffer *command, SDL_GPUTexture *sceneColor,
                     SDL_GPUTexture *sceneDepth, const ModernSsaoView *view);

#endif
