#include "game/asset.h"
#include "game/render.h"
#include "game/render_internal.h"
#include <string.h>

/*
 * Each course draws one stretch of itself from a second page of track
 * textures. Says whether `section` is inside that stretch, both by the flag
 * the uploader watches and by the texture-page bit the caller ORs into its
 * primitives.
 */
s32 SelectTrackTexturePage(s32 section) {
    s32 secondPage = section >= g_TrackTextureSectionLo &&
                     section < g_TrackTextureSectionHi;

    g_TrackTexturePageWanted = secondPage;
    return secondPage ? 0x100 : 0;
}

static void SwapTrackTextureRowAt(s32 row) {
    s32 buffer[0xE0];

    g_TrackTextureRowRect.y = (s16)(row + 0x100);
    if (g_TrackTextureShadowPage[row] != g_TrackTexturePageWanted) return;

    StoreImage(&g_TrackTextureRowRect, buffer);
    DrawSync(0);
    LoadImage(&g_TrackTextureRowRect, g_TrackTextureShadow[row]);
    DrawSync(0);
    memcpy(g_TrackTextureShadow[row], buffer, sizeof(buffer));
    g_TrackTextureShadowPage[row] = 1 - g_TrackTextureShadowPage[row];
}

void SwapTrackTexturePageNow(void) {
    s32 row;

    for (row = 0; row < 0x100; row++) {
        SwapTrackTextureRowAt(row);
    }
}

void SetTrackTexturePageNow(s32 trackSection) {
    g_TrackTextureTargetRow = SelectTrackTexturePage(trackSection);
    g_TrackTextureCursorRow = g_TrackTextureTargetRow;
    SwapTrackTexturePageNow();
}

void ResetTrackTextureSwap(void) {
    s32 i;

    for (i = 0; i < 0x100; i++) {
        g_TrackTextureShadowPage[i] = 1;
    }

    g_TrackTexturePageWanted = 0;
    g_TrackTextureTargetRow = 0;
    g_TrackTextureCursorRow = 0;
}

void RequestTrackTexturePage(s32 trackSection) {
    g_TrackTextureTargetRow = SelectTrackTexturePage(trackSection);
}

void SwapTrackTextureRow(void) {
    SwapTrackTextureRowAt(g_TrackTextureCursorRow);
}

void StepTrackTextureSwap(void) {
    while (g_TrackTextureCursorRow != g_TrackTextureTargetRow) {
        if (VSync(1) >= 471) {
            break;
        }

        if (g_TrackTextureCursorRow < g_TrackTextureTargetRow) {
            SwapTrackTextureRow();
            g_TrackTextureCursorRow++;
        } else {
            g_TrackTextureCursorRow--;
            SwapTrackTextureRow();
        }
    }
}
