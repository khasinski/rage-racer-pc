#ifndef RAGE_EDITOR_UI_H
#define RAGE_EDITOR_UI_H

#include "rage_save.h"

typedef struct EditorState {
    RageSaveFile save;
    RageSaveReport report;
    char path[1024];
    int loaded;
    int dirty;
    int slot;
    RageRegion region;
    int logoColour;   /* the palette entry the brush paints with */
    int logoZoom;
    char status[512];
    /* Names the tab to show first, so a screenshot can pick one. */
    const char *openTab;
} EditorState;

void EditorDrawWindow(EditorState *state);
/* Told by the shell when a file has been chosen, so the panels can react. */
void EditorOpen(EditorState *state, const char *path);
void EditorSave(EditorState *state, const char *path);
void EditorNew(EditorState *state);

#endif
