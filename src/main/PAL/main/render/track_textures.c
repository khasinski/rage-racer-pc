#include "game/asset.h"
#include "game/render.h"
#include "game/render_internal.h"
#include <string.h>

/*
 * Each course draws one stretch of itself from a second page of track
 * textures. Says whether `section` is inside that stretch, both by the flag
 * the uploader watches and by the texture-page bit the caller ORs into its
 * primitives. This query deliberately does not request a texture upload.
 */
s32 TrackTexturePageForSection(s32 section) {
    return section >= g_TrackTextureSectionLo &&
                   section < g_TrackTextureSectionHi
               ? TRACK_TEXTURE_PAGE_ROW_COUNT
               : 0;
}

static void SelectTrackTexturePage(s32 trackSection) {
    g_TrackTextureTargetRow = TrackTexturePageForSection(trackSection);
    g_TrackTexturePageWanted = g_TrackTextureTargetRow != 0;
}

static void SwapTrackTextureRowAt(s32 row) {
    TrackTextureShadowRow buffer;

    g_TrackTextureRowRect.y = (s16)(row + TRACK_TEXTURE_PAGE_ROW_COUNT);
    if (g_TrackTextureShadowPage[row] != g_TrackTexturePageWanted) return;

    StoreImage(&g_TrackTextureRowRect, buffer);
    DrawSync(0);
    LoadImage(&g_TrackTextureRowRect, g_TrackTextureShadow[row]);
    DrawSync(0);
    memcpy(g_TrackTextureShadow[row], buffer, sizeof(buffer));
    g_TrackTextureShadowPage[row] = 1 - g_TrackTextureShadowPage[row];
}

static void SwapAllTrackTextureRows(void) {
    s32 row;

    for (row = 0; row < TRACK_TEXTURE_PAGE_ROW_COUNT; row++) {
        SwapTrackTextureRowAt(row);
    }
}

void SetTrackTexturePageNow(s32 trackSection) {
    SelectTrackTexturePage(trackSection);
    g_TrackTextureCursorRow = g_TrackTextureTargetRow;
    SwapAllTrackTextureRows();
}

void ResetTrackTextureSwap(void) {
    s32 i;

    for (i = 0; i < TRACK_TEXTURE_PAGE_ROW_COUNT; i++) {
        g_TrackTextureShadowPage[i] = 1;
    }

    g_TrackTexturePageWanted = 0;
    g_TrackTextureTargetRow = 0;
    g_TrackTextureCursorRow = 0;
}

void RequestTrackTexturePage(s32 trackSection) {
    SelectTrackTexturePage(trackSection);
}

void StepTrackTextureSwap(void) {
    while (g_TrackTextureCursorRow != g_TrackTextureTargetRow) {
        if (VSync(1) >= 471) {
            break;
        }

        if (g_TrackTextureCursorRow < g_TrackTextureTargetRow) {
            SwapTrackTextureRowAt(g_TrackTextureCursorRow);
            g_TrackTextureCursorRow++;
        } else {
            g_TrackTextureCursorRow--;
            SwapTrackTextureRowAt(g_TrackTextureCursorRow);
        }
    }
}
