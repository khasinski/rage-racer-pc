#ifndef RAGE_EDITOR_UI_H
#define RAGE_EDITOR_UI_H

#include "rage_save.h"

typedef enum EditorSection {
    EDITOR_SECTION_PROGRESS = 0,
    EDITOR_SECTION_GARAGE,
    EDITOR_SECTION_RECORDS,
    EDITOR_SECTION_LOGO,
    EDITOR_SECTION_SETTINGS,
    EDITOR_SECTION_FILE,
    EDITOR_SECTION_ADVANCED,
    EDITOR_SECTION_COUNT
} EditorSection;

typedef struct EditorState {
    RageSaveFile save;
    RageSaveReport report;
    char path[1024];
    int loaded;
    int dirty;
    int slot;
    RageRegion region;
    int logoColour;
    int logoZoom;
    char status[512];
    int statusIsError;
    EditorSection section;
    const char *openTab; /* a screenshot can ask for one section by name */

    RageSaveEntry found[RAGE_SAVE_DISCOVER_MAX];
    int foundCount;
    int scanned;
} EditorState;

/* Set by the shell so the panels can ask for a dialog without knowing SDL. */
typedef struct EditorRequests {
    int open;
    int saveAs;
} EditorRequests;

void EditorDrawWindow(EditorState *state, EditorRequests *requests);
void EditorOpen(EditorState *state, const char *path);
void EditorSave(EditorState *state, const char *path);
void EditorNew(EditorState *state);
void EditorRescan(EditorState *state);
/* The name this save wants on a card, from its release and slot. */
void EditorSuggestedName(const EditorState *state, char *out, size_t size);

#endif
