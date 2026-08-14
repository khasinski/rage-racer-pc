#include "common.h"
#include "game/render.h"
#include "game/render_internal.h"

void LoadTrackTexturePageRange(void) {
    g_TrackTextureSectionLo = g_TrackRenderTable->textureSectionLo;
    g_TrackTextureSectionHi = g_TrackRenderTable->textureSectionHi;
}
