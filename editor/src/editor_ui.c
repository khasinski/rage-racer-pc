/*
 * Every field a Rage Racer save holds, laid out so it can be found.
 *
 * The panels follow the save's own structure rather than a tidier one, so a
 * field here can be traced to a field in GameSaveBlock without a lookup table
 * in between. Nothing is hidden: what the game does not use is in the last
 * tab as bytes rather than left out, because a save editor that quietly drops
 * a field writes files that lose it.
 */

#include "editor_ui.h"

#include "cimgui.h"

#include <stdio.h>
#include <string.h>

static const ImVec2_c kAuto = {0.0f, 0.0f};

/* Times are milliseconds. The game shows them as 1'40"765. */
static void FormatTime(int milliseconds, char *out, size_t size) {
    int negative = milliseconds < 0;
    long value = negative ? -(long)milliseconds : milliseconds;
    long minutes = value / 60000;
    long seconds = (value / 1000) % 60;
    long thousandths = value % 1000;

    snprintf(out, size, "%s%ld'%02ld\"%03ld", negative ? "-" : "", minutes,
             seconds, thousandths);
}

/* A number and, beside it, what that number means on screen. */
static int TimeField(const char *label, int *milliseconds) {
    char shown[32];
    int changed = igInputInt(label, milliseconds, 1, 1000, 0);

    FormatTime(*milliseconds, shown, sizeof(shown));
    igSameLine(0.0f, -1.0f);
    igText("%s", shown);
    return changed;
}

static int ByteField(const char *label, unsigned char *value) {
    int wide = *value;
    if (igInputInt(label, &wide, 1, 10, 0)) {
        if (wide < 0) wide = 0;
        if (wide > 255) wide = 255;
        *value = (unsigned char)wide;
        return 1;
    }
    return 0;
}

static int HalfwordField(const char *label, unsigned short *value) {
    int wide = *value;
    if (igInputInt(label, &wide, 1, 100, 0)) {
        if (wide < 0) wide = 0;
        if (wide > 0xFFFF) wide = 0xFFFF;
        *value = (unsigned short)wide;
        return 1;
    }
    return 0;
}

static int SignedHalfwordField(const char *label, short *value) {
    int wide = *value;
    if (igInputInt(label, &wide, 1, 100, 0)) {
        if (wide < -32768) wide = -32768;
        if (wide > 32767) wide = 32767;
        *value = (short)wide;
        return 1;
    }
    return 0;
}

static int WordField(const char *label, int *value) {
    return igInputInt(label, value, 1, 100, 0);
}

/* Opens the tab a screenshot asked for, once. */
static ImGuiTabItemFlags TabFlags(EditorState *state, const char *name) {
    if (state->openTab != NULL && strcmp(state->openTab, name) == 0)
        return ImGuiTabItemFlags_SetSelected;
    return 0;
}

static void DrawFilePanel(EditorState *state) {
    const RageRegionInfo *regions;
    size_t regionCount;
    size_t i;
    char name[64];
    char team[RAGE_TEAM_NAME_LENGTH + 1];

    igSeparatorText("File");
    igText("%s", state->path[0] != '\0' ? state->path : "(nothing open)");

    igSeparatorText("Release");
    regions = RageRegionTable(&regionCount);
    for (i = 0; i < regionCount; i++) {
        char label[96];
        snprintf(label, sizeof(label), "%s  (%s)%s", regions[i].name,
                 regions[i].serial,
                 regions[i].verified ? "" : "  [serial unconfirmed]");
        if (igRadioButton_Bool(label, state->region == regions[i].region)) {
            state->region = regions[i].region;
            state->dirty = 1;
        }
    }
    igText("The release changes the name the file needs on a memory card, and "
           "nothing about the fields below: all three store the same save.");

    igSeparatorText("Slot");
    if (igSliderInt("slot", &state->slot, 0, RAGE_SAVE_SLOTS - 1, "%d", 0))
        state->dirty = 1;
    if (RageRegionCardName(state->region, state->slot, name, sizeof(name)))
        igText("On a card this file is called  %s", name);

    igSeparatorText("Team");
    RageSaveReadTeamName(&state->save.header, team, sizeof(team));
    if (igInputText("team name", team, sizeof(team), 0, NULL, NULL)) {
        RageSaveWriteTeamName(&state->save.header, team);
        state->dirty = 1;
    }
    igText("Up to seven of  %s", kRageNameCharset);
    {
        int counter = state->save.header.fields.saveCounter;
        if (WordField("save counter", &counter)) {
            state->save.header.fields.saveCounter = counter;
            state->dirty = 1;
        }
    }

    igSeparatorText("Checksums");
    igText("These are recomputed when the file is written, so a save that "
           "arrived damaged leaves here repaired.");
    igText("memory card signature: %s",
           state->report.iconRecognised ? "present" : "MISSING");
    igText("header: %s   payload: %s   repeated header: %s   copies match: %s",
           state->report.headerChecksumValid ? "ok" : "WRONG",
           state->report.blockChecksumValid ? "ok" : "WRONG",
           state->report.trailerChecksumValid ? "ok" : "WRONG",
           state->report.trailerMatchesHeader ? "yes" : "NO");
}

static int DrawProgressGroup(const char *label, SavedRaceProgress *progress) {
    int changed = 0;
    char id[64];

    igPushID_Str(label);
    igSeparatorText(label);
    snprintf(id, sizeof(id), "course");
    changed |= WordField(id, &progress->course);
    changed |= WordField("car", &progress->carIndex);
    changed |= WordField("class", &progress->classIndex);
    changed |= WordField("highest class reached", &progress->maxClassReached);
    changed |= WordField("money", &progress->money);
    igPopID();
    return changed;
}

static void DrawProgressPanel(EditorState *state) {
    GameSaveBlock *block = &state->save.block;
    int i;

    state->dirty |= DrawProgressGroup("Grand Prix", &block->grandPrixProgress);
    state->dirty |=
        DrawProgressGroup("Extra Grand Prix", &block->extraGrandPrixProgress);
    state->dirty |=
        DrawProgressGroup("Time Attack", &block->timeAttackProgress);

    igSeparatorText("Unlocks");
    state->dirty |= HalfwordField("extra grand prix unlocked",
                                  &block->extraGrandPrixUnlocked);
    for (i = 0; i < 2; i++) {
        char label[64];
        snprintf(label, sizeof(label), "highest class reached [%d]", i);
        state->dirty |= WordField(label, &block->maxClassReached[i]);
    }

    igSeparatorText("Best place per course");
    igText("One byte a course, as the game records the place finished.");
    if (igBeginTable("course progress", 9, ImGuiTableFlags_Borders, kAuto,
                     0.0f)) {
        int row;
        igTableSetupColumn("series", 0, 0.0f, 0);
        for (i = 0; i < 8; i++) {
            char header[16];
            snprintf(header, sizeof(header), "%d", i);
            igTableSetupColumn(header, 0, 0.0f, 0);
        }
        igTableHeadersRow();
        for (row = 0; row < 2; row++) {
            unsigned char *bytes = row == 0 ? block->grandPrixCourseProgress
                                            : block->extraGrandPrixCourseProgress;
            igTableNextRow(0, 0.0f);
            igTableNextColumn();
            igText("%s", row == 0 ? "grand prix" : "extra");
            for (i = 0; i < 8; i++) {
                char id[32];
                igTableNextColumn();
                snprintf(id, sizeof(id), "##progress%d%d", row, i);
                igSetNextItemWidth(60.0f);
                state->dirty |= ByteField(id, &bytes[i]);
            }
        }
        igEndTable();
    }
}

static void DrawGaragePanel(EditorState *state) {
    GameSaveBlock *block = &state->save.block;
    int garage;

    igText("Three sets of thirteen cars: one for the grand prix, one for the "
           "extra grand prix and one for time attack.");
    for (garage = 0; garage < 3; garage++) {
        static const char *const kNames[3] = {"Grand Prix", "Extra Grand Prix",
                                              "Time Attack"};
        char id[32];
        snprintf(id, sizeof(id), "garage%d", garage);
        igSeparatorText(kNames[garage]);
        if (igBeginTable(id, 7, ImGuiTableFlags_Borders |
                                    ImGuiTableFlags_SizingFixedFit,
                         kAuto, 0.0f)) {
            int car;
            igTableSetupColumn("car", 0, 0.0f, 0);
            igTableSetupColumn("grade", 0, 0.0f, 0);
            igTableSetupColumn("tyres", 0, 0.0f, 0);
            igTableSetupColumn("gearbox", 0, 0.0f, 0);
            igTableSetupColumn("paint 1", 0, 0.0f, 0);
            igTableSetupColumn("paint 2", 0, 0.0f, 0);
            igTableSetupColumn("owned", 0, 0.0f, 0);
            igTableHeadersRow();
            for (car = 0; car < 13; car++) {
                SavedCarSetup *setup = &block->carSetup[garage][car];
                bool owned = setup->enabled != 0;
                char field[32];
                igTableNextRow(0, 0.0f);
                igTableNextColumn();
                igText("%d", car);
                igTableNextColumn();
                snprintf(field, sizeof(field), "##g%d%d", garage, car);
                igSetNextItemWidth(70.0f);
                state->dirty |= ByteField(field, &setup->modelVariant);
                igTableNextColumn();
                snprintf(field, sizeof(field), "##t%d%d", garage, car);
                igSetNextItemWidth(70.0f);
                state->dirty |= ByteField(field, &setup->tireCompound);
                igTableNextColumn();
                snprintf(field, sizeof(field), "##x%d%d", garage, car);
                igSetNextItemWidth(70.0f);
                state->dirty |= ByteField(field, &setup->transmission);
                igTableNextColumn();
                snprintf(field, sizeof(field), "##p%d%d", garage, car);
                igSetNextItemWidth(70.0f);
                state->dirty |= ByteField(field, &setup->paintColor1);
                igTableNextColumn();
                snprintf(field, sizeof(field), "##q%d%d", garage, car);
                igSetNextItemWidth(70.0f);
                state->dirty |= ByteField(field, &setup->paintColor2);
                igTableNextColumn();
                snprintf(field, sizeof(field), "##o%d%d", garage, car);
                if (igCheckbox(field, &owned)) {
                    setup->enabled = owned ? 1 : 0;
                    state->dirty = 1;
                }
            }
            igEndTable();
        }
    }
}

static void DrawRecordTable(EditorState *state, const char *id,
                            RaceRecord records[4][5]) {
    int course;

    if (!igBeginTable(id, 4, ImGuiTableFlags_Borders, kAuto, 0.0f)) return;
    igTableSetupColumn("course", 0, 0.0f, 0);
    igTableSetupColumn("place", 0, 0.0f, 0);
    igTableSetupColumn("driver", 0, 0.0f, 0);
    igTableSetupColumn("time", 0, 0.0f, 0);
    igTableHeadersRow();
    for (course = 0; course < 4; course++) {
        int place;
        for (place = 0; place < 5; place++) {
            RaceRecord *record = &records[course][place];
            char name[sizeof(record->driverName) + 1];
            char field[48];
            int milliseconds = record->raceTime;

            igTableNextRow(0, 0.0f);
            igTableNextColumn();
            if (place == 0) igText("%d", course);
            igTableNextColumn();
            igText("%d", place + 1);
            igTableNextColumn();
            memcpy(name, record->driverName, sizeof(record->driverName));
            name[sizeof(record->driverName)] = '\0';
            snprintf(field, sizeof(field), "##%s_n%d%d", id, course, place);
            igSetNextItemWidth(110.0f);
            if (igInputText(field, name, sizeof(name), 0, NULL, NULL)) {
                /* The field is eight bytes and is not terminated. */
                memset(record->driverName, 0, sizeof(record->driverName));
                memcpy(record->driverName, name,
                       strnlen(name, sizeof(record->driverName)));
                state->dirty = 1;
            }
            igTableNextColumn();
            snprintf(field, sizeof(field), "##%s_t%d%d", id, course, place);
            igSetNextItemWidth(120.0f);
            if (TimeField(field, &milliseconds)) {
                record->raceTime = milliseconds;
                state->dirty = 1;
            }
            igSameLine(0.0f, -1.0f);
            {
                int carIndex = record->carIndex;
                snprintf(field, sizeof(field), "car##%s_c%d%d", id, course,
                         place);
                igSetNextItemWidth(80.0f);
                if (igInputInt(field, &carIndex, 1, 1, 0)) {
                    record->carIndex = (short)carIndex;
                    state->dirty = 1;
                }
            }
        }
    }
    igEndTable();
}

static void DrawRecordsPanel(EditorState *state) {
    GameSaveBlock *block = &state->save.block;
    int i;

    igSeparatorText("Class records");
    if (igBeginTable("class records", 3, ImGuiTableFlags_Borders, kAuto,
                     0.0f)) {
        igTableSetupColumn("class", 0, 0.0f, 0);
        igTableSetupColumn("grade", 0, 0.0f, 0);
        igTableSetupColumn("clears", 0, 0.0f, 0);
        igTableHeadersRow();
        for (i = 0; i < 11; i++) {
            char field[32];
            igTableNextRow(0, 0.0f);
            igTableNextColumn();
            igText("%d", i);
            igTableNextColumn();
            snprintf(field, sizeof(field), "##cg%d", i);
            igSetNextItemWidth(90.0f);
            state->dirty |= HalfwordField(field, &block->classRecords[i].grade);
            igTableNextColumn();
            snprintf(field, sizeof(field), "##cc%d", i);
            igSetNextItemWidth(90.0f);
            state->dirty |= HalfwordField(field, &block->classRecords[i].clears);
        }
        igEndTable();
    }

    igSeparatorText("Best times");
    igText("Two sets of four courses. The game keeps two laps, two totals and "
           "three sector times for each.");
    if (igBeginTable("best times", 8, ImGuiTableFlags_Borders, kAuto, 0.0f)) {
        int set;
        igTableSetupColumn("set", 0, 0.0f, 0);
        igTableSetupColumn("course", 0, 0.0f, 0);
        igTableSetupColumn("lap 1", 0, 0.0f, 0);
        igTableSetupColumn("lap 2", 0, 0.0f, 0);
        igTableSetupColumn("total 1", 0, 0.0f, 0);
        igTableSetupColumn("total 2", 0, 0.0f, 0);
        igTableSetupColumn("sector 1", 0, 0.0f, 0);
        igTableSetupColumn("sectors 2,3", 0, 0.0f, 0);
        igTableHeadersRow();
        for (set = 0; set < 2; set++) {
            int course;
            for (course = 0; course < 4; course++) {
                char field[48];
                int slot;
                igTableNextRow(0, 0.0f);
                igTableNextColumn();
                igText("%d", set);
                igTableNextColumn();
                igText("%d", course);
                for (slot = 0; slot < 2; slot++) {
                    igTableNextColumn();
                    snprintf(field, sizeof(field), "##l%d%d%d", set, course,
                             slot);
                    igSetNextItemWidth(150.0f);
                    state->dirty |=
                        TimeField(field, &block->bestLapTimes[set][course][slot]);
                }
                for (slot = 0; slot < 2; slot++) {
                    igTableNextColumn();
                    snprintf(field, sizeof(field), "##T%d%d%d", set, course,
                             slot);
                    igSetNextItemWidth(150.0f);
                    state->dirty |= TimeField(
                        field, &block->bestTotalTimes[set][course][slot]);
                }
                igTableNextColumn();
                snprintf(field, sizeof(field), "##s%d%d0", set, course);
                igSetNextItemWidth(150.0f);
                state->dirty |=
                    TimeField(field, &block->bestSectorTimes[set][course][0]);
                igTableNextColumn();
                for (slot = 1; slot < 3; slot++) {
                    snprintf(field, sizeof(field), "##s%d%d%d", set, course,
                             slot);
                    igSetNextItemWidth(150.0f);
                    state->dirty |= TimeField(
                        field, &block->bestSectorTimes[set][course][slot]);
                }
            }
        }
        igEndTable();
    }

    igSeparatorText("Ranking records");
    for (i = 0; i < 2; i++) {
        char id[32];
        snprintf(id, sizeof(id), "ranking set %d", i);
        igSeparatorText(id);
        snprintf(id, sizeof(id), "ranking%d", i);
        DrawRecordTable(state, id, state->save.block.rankingRecords[i]);
    }
    igSeparatorText("Time attack records");
    for (i = 0; i < 2; i++) {
        char id[32];
        snprintf(id, sizeof(id), "time set %d", i);
        igSeparatorText(id);
        snprintf(id, sizeof(id), "time%d", i);
        DrawRecordTable(state, id, state->save.block.timeRecords[i]);
    }
}

static void DrawLogoPanel(EditorState *state) {
    GameSaveBlock *block = &state->save.block;
    ImDrawList *draw;
    ImVec2_c origin;
    float cell;
    int x, y;
    int i;

    igText("Sixty four by sixty four, sixteen colours, four pixels to a "
           "halfword. This is the logo the game paints on the car.");
    igSliderInt("zoom", &state->logoZoom, 2, 12, "%d", 0);

    igSeparatorText("Palette");
    for (i = 0; i < 16; i++) {
        unsigned char rgb[3];
        int transparent;
        float colour[3];
        char label[32];

        RageLogoColour(block->teamLogoClut[i], rgb, &transparent);
        colour[0] = rgb[0] / 255.0f;
        colour[1] = rgb[1] / 255.0f;
        colour[2] = rgb[2] / 255.0f;
        snprintf(label, sizeof(label), "##clut%d", i);
        igSetNextItemWidth(120.0f);
        if (igColorEdit3(label, colour,
                         ImGuiColorEditFlags_NoInputs |
                             ImGuiColorEditFlags_NoLabel)) {
            unsigned char packed[3];
            packed[0] = (unsigned char)(colour[0] * 255.0f);
            packed[1] = (unsigned char)(colour[1] * 255.0f);
            packed[2] = (unsigned char)(colour[2] * 255.0f);
            block->teamLogoClut[i] = RageLogoPackColour(packed, transparent);
            state->dirty = 1;
        }
        igSameLine(0.0f, -1.0f);
        snprintf(label, sizeof(label), "%2d##pick%d", i, i);
        if (igRadioButton_Bool(label, state->logoColour == i))
            state->logoColour = i;
        igSameLine(0.0f, -1.0f);
        {
            bool clear = transparent != 0;
            snprintf(label, sizeof(label), "see through##t%d", i);
            if (igCheckbox(label, &clear)) {
                block->teamLogoClut[i] = RageLogoPackColour(rgb, clear ? 1 : 0);
                state->dirty = 1;
            }
        }
        if ((i % 2) == 0) igSameLine(320.0f, -1.0f);
    }

    igSeparatorText("Canvas");
    igText("Left button paints the chosen colour, right button picks one up.");
    cell = (float)state->logoZoom;
    origin = igGetCursorScreenPos();
    draw = igGetWindowDrawList();
    for (y = 0; y < RAGE_LOGO_HEIGHT; y++) {
        for (x = 0; x < RAGE_LOGO_WIDTH; x++) {
            unsigned char rgb[3];
            int transparent;
            ImVec2_c min;
            ImVec2_c max;
            unsigned int packed;

            RageLogoColour(block->teamLogoClut[RageLogoPixel(block, x, y)], rgb,
                           &transparent);
            min.x = origin.x + x * cell;
            min.y = origin.y + y * cell;
            max.x = min.x + cell;
            max.y = min.y + cell;
            /* A see-through entry is shown as a chequer rather than a colour,
             * so it cannot be mistaken for black. */
            if (transparent) {
                packed = ((x + y) & 1) ? 0xFF404040u : 0xFF303030u;
            } else {
                packed = 0xFF000000u | ((unsigned int)rgb[2] << 16) |
                         ((unsigned int)rgb[1] << 8) | rgb[0];
            }
            ImDrawList_AddRectFilled(draw, min, max, packed, 0.0f, 0);
        }
    }
    {
        ImVec2_c size;
        ImVec2_c mouse;
        size.x = RAGE_LOGO_WIDTH * cell;
        size.y = RAGE_LOGO_HEIGHT * cell;
        igInvisibleButton("canvas", size, 0);
        mouse = igGetMousePos();
        if (igIsItemHovered(0)) {
            int px = (int)((mouse.x - origin.x) / cell);
            int py = (int)((mouse.y - origin.y) / cell);
            if (px >= 0 && py >= 0 && px < RAGE_LOGO_WIDTH &&
                py < RAGE_LOGO_HEIGHT) {
                if (igIsMouseDown_Nil(ImGuiMouseButton_Left)) {
                    RageLogoSetPixel(block, px, py, state->logoColour);
                    state->dirty = 1;
                } else if (igIsMouseDown_Nil(ImGuiMouseButton_Right)) {
                    state->logoColour = RageLogoPixel(block, px, py);
                }
            }
        }
    }
}

static void DrawControlsPanel(EditorState *state) {
    GameSaveBlock *block = &state->save.block;

    igSeparatorText("Controller");
    state->dirty |= HalfwordField("pad mapping", &block->padMappingIndex);
    state->dirty |= HalfwordField("neGcon mapping", &block->negconMappingIndex);
    state->dirty |= HalfwordField("neGcon steering centre",
                                  &block->negconSteerNeutral);
    state->dirty |= HalfwordField("neGcon steering play",
                                  &block->negconSteerPlay);
    state->dirty |= HalfwordField("neGcon I centre", &block->negconNeutralI);
    state->dirty |= HalfwordField("neGcon II centre", &block->negconNeutralII);
    state->dirty |= HalfwordField("neGcon L centre", &block->negconNeutralL);
    state->dirty |= HalfwordField("neGcon full twist", &block->negconMaxTwist);

    igSeparatorText("Sound");
    state->dirty |= SignedHalfwordField("music track", &block->bgmSelection);
    state->dirty |= WordField("music volume", &block->bgmVolume);
    state->dirty |= WordField("effects volume", &block->sfxVolume);
    {
        bool mono = block->monoOutput != 0;
        if (igCheckbox("mono", &mono)) {
            block->monoOutput = mono ? 1 : 0;
            state->dirty = 1;
        }
    }
}

/* Whatever the game does not read, shown rather than dropped. */
static void DrawRawPanel(EditorState *state) {
    unsigned int i;

    igSeparatorText("Unused payload bytes");
    igText("The save keeps 0x24 bytes the game never reads. They are here so "
           "that a file edited by this program keeps whatever it arrived "
           "with.");
    for (i = 0; i < sizeof(state->save.block.reserved); i++) {
        char label[32];
        snprintf(label, sizeof(label), "##r%u", i);
        igSetNextItemWidth(60.0f);
        state->dirty |= ByteField(label, &state->save.block.reserved[i]);
        if ((i % 12) != 11) igSameLine(0.0f, -1.0f);
    }

    igSeparatorText("Icon block");
    igText("The first 0x200 bytes: the card signature, the title the console "
           "shows, and the sixteen by sixteen icon. Kept as it was found.");
    igText("signature %c%c   frames %02X   blocks %02X", state->save.icon[0],
           state->save.icon[1], state->save.icon[2], state->save.icon[3]);
}

void EditorDrawWindow(EditorState *state) {
    if (!igBeginTabBar("tabs", 0)) return;
    if (igBeginTabItem("File", NULL, TabFlags(state, "File"))) {
        DrawFilePanel(state);
        igEndTabItem();
    }
    if (state->loaded) {
        if (igBeginTabItem("Progress", NULL, TabFlags(state, "Progress"))) {
            DrawProgressPanel(state);
            igEndTabItem();
        }
        if (igBeginTabItem("Garage", NULL, TabFlags(state, "Garage"))) {
            DrawGaragePanel(state);
            igEndTabItem();
        }
        if (igBeginTabItem("Records", NULL, TabFlags(state, "Records"))) {
            DrawRecordsPanel(state);
            igEndTabItem();
        }
        if (igBeginTabItem("Team logo", NULL, TabFlags(state, "Team logo"))) {
            DrawLogoPanel(state);
            igEndTabItem();
        }
        if (igBeginTabItem("Controls and sound", NULL, TabFlags(state, "Controls and sound"))) {
            DrawControlsPanel(state);
            igEndTabItem();
        }
        if (igBeginTabItem("Everything else", NULL, TabFlags(state, "Everything else"))) {
            DrawRawPanel(state);
            igEndTabItem();
        }
    }
    igEndTabBar();
}

void EditorOpen(EditorState *state, const char *path) {
    if (RageSaveLoad(path, &state->save, &state->report)) {
        snprintf(state->path, sizeof(state->path), "%s", path);
        state->loaded = 1;
        state->dirty = 0;
        if (state->report.region != RAGE_REGION_UNKNOWN)
            state->region = state->report.region;
        snprintf(state->status, sizeof(state->status), "opened %s", path);
    } else {
        state->loaded = 0;
        snprintf(state->status, sizeof(state->status), "%s",
                 state->report.detail);
    }
}

void EditorSave(EditorState *state, const char *path) {
    if (RageSaveStore(path, &state->save, &state->report)) {
        snprintf(state->path, sizeof(state->path), "%s", path);
        state->dirty = 0;
        snprintf(state->status, sizeof(state->status),
                 "written to %s, checksums recomputed", path);
    } else {
        snprintf(state->status, sizeof(state->status), "%s",
                 state->report.detail);
    }
}

void EditorNew(EditorState *state) {
    RageSaveInit(&state->save, state->region, state->slot);
    RageSaveCheck(&state->save, &state->report);
    state->report.status = RAGE_SAVE_OK;
    state->loaded = 1;
    state->dirty = 1;
    state->path[0] = '\0';
    snprintf(state->status, sizeof(state->status),
             "new save, not written anywhere yet");
}
