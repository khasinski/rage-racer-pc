#include "game/car.h"
#include "game/state.h"
#include "game/random.h"
#include "game/asset.h"
#include "game/render.h"

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

void SwapTrackTexturePageNow(void) {
    s32 buffer[0xE0];
    s32 page = 0;
    s16 *rectY = &g_TrackTextureRowRect.y;
    Rect *rect = &g_TrackTextureRowRect;
    TrackTextureShadowRow **basePtr = &g_TrackTextureShadow;
    s32 value;
    s32 *src;
    s32 *dst;
    s32 count;

    do {
        *rectY = page + 0x100;
        value = 1 - g_TrackTextureShadowPage[page];
        if (g_TrackTextureShadowPage[page] == g_TrackTexturePageWanted) {
            StoreImage(rect, buffer);
            DrawSync(0);
            LoadImage(rect, (*basePtr)[page]);
            DrawSync(0);

            src = buffer;
            dst = (*basePtr)[page];
            count = 0;
            do {
                *dst++ = *src++;
                count++;
            } while (count < 0xE0);

            g_TrackTextureShadowPage[page] = value;
        }
        page++;
    } while (page < 0x100);
}

void SetTrackTexturePageNow(s32 trackSection) {
    s32 temp;

    temp = SelectTrackTexturePage(trackSection);
    g_TrackTextureTargetRow = temp;
    g_TrackTextureCursorRow = temp;
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
    s32 buffer[0xE0];
    s16 *rectY;
    s32 value;
    s32 one;
    s32 *dst;
    s32 *src;
    s32 count;
    TrackTextureShadowRow **basePtr;
    Rect *rect;
    s32 copyIndex;
    s32 index;

    rectY = &g_TrackTextureRowRect.y;
    *rectY = (u16)g_TrackTextureCursorRow + 0x100;
    one = 1;
    value = one - g_TrackTextureShadowPage[g_TrackTextureCursorRow];
    if (g_TrackTextureShadowPage[g_TrackTextureCursorRow] == g_TrackTexturePageWanted) {
        rect = &g_TrackTextureRowRect;
        StoreImage(rect, buffer);
        DrawSync(0);

        index = g_TrackTextureCursorRow;
        rect = &g_TrackTextureRowRect;
        basePtr = &g_TrackTextureShadow;
        LoadImage(rect, (*basePtr)[index]);
        DrawSync(0);

        src = buffer;
        copyIndex = g_TrackTextureCursorRow;
        count = 0;
        dst = (*basePtr)[copyIndex];
        do {
            *dst++ = *src++;
            count++;
        } while (count < 0xE0);

        g_TrackTextureShadowPage[g_TrackTextureCursorRow] = value;
    }
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

s32 CycleBgmSelectCameraCar(s32 mask, s32 current) {
    s32 random;
    s32 candidate;
    s32 first;

    if (mask & g_SceneTimer) {
        return current;
    }
    if ((g_TrackTextureCursorRow == 0) || (g_TrackTextureCursorRow == 0x100)) {
        random = Random15() & 0x7FFF;
        candidate = random % 11;

        first = SelectTrackTexturePage(g_Cars[current].trackSection);

        if (first == SelectTrackTexturePage(g_Cars[candidate].trackSection)) {
            return candidate;
        }
    }
    return current;
}

s32 CycleAttractCameraCar(s32 mask, s32 current) {
    s32 random;
    s32 candidate;
    s32 first;

    if (mask & g_SceneTimer) {
        return current;
    }
    if ((g_TrackTextureCursorRow == 0) || (g_TrackTextureCursorRow == 0x100)) {
        random = Random15() & 0x7FFF;
        candidate = random % 4;

        first = SelectTrackTexturePage(g_Cars[current].trackSection);

        if (first == SelectTrackTexturePage(g_Cars[candidate].trackSection)) {
            return candidate;
        }
    }
    return current;
}
