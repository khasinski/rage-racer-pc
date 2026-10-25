#ifndef RAGE_PORT_OVERLAY_RASTER_H
#define RAGE_PORT_OVERLAY_RASTER_H

#include <stdint.h>

#include "overlay_model.h"
#include "touch_model.h"

/* Draws the settings panel into a premultiplied RGBA image on the CPU. It
 * knows the model and the layout, not SDL or the GPU, so what it paints can
 * be checked pixel by pixel. */

typedef struct OverlaySurface {
    uint8_t *pixels;
    int w;
    int h; /* includes one extra bottom row, see OverlayPaint */
} OverlaySurface;

/* Sizes the surface for a panel of width x height pixels. The image keeps one
 * row more: a solid texel the GPU backdrop quad samples. */
int OverlaySurfaceResize(OverlaySurface *surface, int width, int height);
void OverlaySurfaceRelease(OverlaySurface *surface);

/* The bundled typeface loads on first use. */
int OverlayFontReady(void);

/* Repaints the whole image, solid row included. */
void OverlayPaint(OverlaySurface *surface, const OverlayModel *model,
                  const OverlayLayout *layout);

/* Paints the touch pad: for each control a tile of the idle picture and one
 * of the pressed picture, placed as TouchAtlasOrigin says. The surface is the
 * size TouchAtlasSize gives. */
void OverlayPaintTouchAtlas(OverlaySurface *surface, const TouchLayout *layout);

/* Writes the panel as a PPM over mid-grey, for looking at it. */
int OverlaySurfaceDump(const OverlaySurface *surface, const char *path);

#endif
