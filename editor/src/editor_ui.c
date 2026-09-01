/*
 * The editor's screen.
 *
 * A header saying what is open, a list of sections down the side and the
 * fields in the middle: the shape of an ordinary program rather than a list
 * of every field the save happens to contain. What a field means is shown
 * where the game gives it a meaning, and as a plain number where it does not,
 * because a made-up name is worse than a number.
 */

#include "editor_ui.h"

#include "cimgui.h"

#include <stdio.h>
#include <string.h>

static const ImVec2_c kAuto = {0.0f, 0.0f};

static ImVec2_c Size(float x, float y) {
    ImVec2_c value;
    value.x = x;
    value.y = y;
    return value;
}

/* An explanation the reader can ask for instead of one shouted at them. */
static void Explain(const char *text) {
    igSameLine(0.0f, -1.0f);
    igTextDisabled("(?)");
    if (igBeginItemTooltip()) {
        igPushTextWrapPos(igGetFontSize() * 28.0f);
        igTextUnformatted(text, NULL);
        igPopTextWrapPos();
        igEndTooltip();
    }
}

static void Heading(const char *text) {
    igDummy(Size(0.0f, 6.0f));
    igSeparatorText(text);
}

static void FormatMoney(int money, char *out, size_t size) {
    char digits[16];
    int length;
    int i;
    int written = 0;

    snprintf(digits, sizeof(digits), "%d", money < 0 ? -money : money);
    length = (int)strlen(digits);
    if (money < 0 && written + 1 < (int)size) out[written++] = '-';
    for (i = 0; i < length; i++) {
        if (i > 0 && ((length - i) % 3) == 0 && written + 1 < (int)size)
            out[written++] = ' ';
        if (written + 1 < (int)size) out[written++] = digits[i];
    }
    out[written] = '\0';
}

static void FormatTime(int milliseconds, char *out, size_t size) {
    int negative = milliseconds < 0;
    long value = negative ? -(long)milliseconds : milliseconds;

    snprintf(out, size, "%s%ld'%02ld\"%03ld", negative ? "-" : "",
             value / 60000, (value / 1000) % 60, value % 1000);
}

/*
 * A time as the game writes it, edited as the game writes it. Typing
 * 1'40"765 is easier than remembering it is 100765 milliseconds.
 */
static int TimeField(const char *label, int *milliseconds, float width) {
    char text[32];
    int minutes = 0;
    long seconds = 0;
    long thousandths = 0;

    FormatTime(*milliseconds, text, sizeof(text));
    igSetNextItemWidth(width);
    if (!igInputText(label, text, sizeof(text), 0, NULL, NULL)) return 0;
    if (sscanf(text, "%d'%ld\"%ld", &minutes, &seconds, &thousandths) == 3 ||
        sscanf(text, "%d:%ld.%ld", &minutes, &seconds, &thousandths) == 3) {
        *milliseconds =
            (int)((long)minutes * 60000 + seconds * 1000 + thousandths);
        return 1;
    }
    /* A bare number is still a number of milliseconds. */
    if (sscanf(text, "%d", milliseconds) == 1) return 1;
    return 0;
}

static int ByteSlider(const char *label, unsigned char *value, int high,
                      float width) {
    int wide = *value;
    igSetNextItemWidth(width);
    if (igSliderInt(label, &wide, 0, high, "%d", 0)) {
        *value = (unsigned char)wide;
        return 1;
    }
    return 0;
}

static int WordField(const char *label, int *value, float width) {
    igSetNextItemWidth(width);
    return igInputInt(label, value, 0, 0, 0);
}

static int HalfwordField(const char *label, unsigned short *value,
                         float width) {
    int wide = *value;
    igSetNextItemWidth(width);
    if (igInputInt(label, &wide, 0, 0, 0)) {
        if (wide < 0) wide = 0;
        if (wide > 0xFFFF) wide = 0xFFFF;
        *value = (unsigned short)wide;
        return 1;
    }
    return 0;
}

/* Five of the thirteen are called something else in Japan, so the list
 * follows whichever release the file belongs to. */
static void CarLabel(const EditorState *state, int index, char *out,
                     size_t size) {
    const char *name = RageCarName(index, state->region);
    const char *maker = RageCarMaker(index);

    if (name != NULL && maker != NULL)
        snprintf(out, size, "%s %s", maker, name);
    else
        snprintf(out, size, "car %d", index);
}

/* ------------------------------------------------------------------ */

static void DrawWelcome(EditorState *state, EditorRequests *requests) {
    int i;

    igDummy(Size(0.0f, 20.0f));
    igPushFont(NULL, igGetFontSize() * 1.6f);
    igText("Rage Racer save editor");
    igPopFont();
    igTextDisabled("Open one of your saves, or start a new one.");
    igDummy(Size(0.0f, 16.0f));

    if (!state->scanned) EditorRescan(state);

    if (state->foundCount > 0) {
        igSeparatorText("Saves on this computer");
        for (i = 0; i < state->foundCount; i++) {
            RageSaveEntry *entry = &state->found[i];
            const RageRegionInfo *info = RageRegionFind(entry->region);
            char money[24];
            char label[512];

            FormatMoney(entry->money, money, sizeof(money));
            snprintf(label, sizeof(label), "%s%s##found%d",
                     entry->team[0] != '\0' ? entry->team : "(no team name)",
                     entry->valid ? "" : "   damaged", i);
            if (igButton(label, Size(360.0f, 0.0f))) EditorOpen(state, entry->path);
            igSameLine(0.0f, 12.0f);
            igBeginGroup();
            igTextDisabled("%s   slot %d   %s credits",
                           info != NULL ? info->name : "unknown release",
                           entry->slot, money);
            igTextDisabled("%s", entry->path);
            igEndGroup();
            igDummy(Size(0.0f, 4.0f));
        }
    } else {
        igSeparatorText("Saves on this computer");
        igTextDisabled("None found. The game writes them here once you save:");
        {
            char directory[1024];
            if (RageSaveCardDirectory(0, directory, sizeof(directory)))
                igTextDisabled("%s", directory);
        }
    }

    igDummy(Size(0.0f, 16.0f));
    if (igButton("Open a file...", Size(200.0f, 40.0f))) requests->open = 1;
    igSameLine(0.0f, 12.0f);
    if (igButton("Start a new save", Size(200.0f, 40.0f))) EditorNew(state);
    igSameLine(0.0f, 12.0f);
    if (igButton("Look again", Size(140.0f, 40.0f))) EditorRescan(state);
}

/* ------------------------------------------------------------------ */

static void DrawProgress(EditorState *state) {
    GameSaveBlock *block = &state->save.block;
    static const char *const kNames[3] = {"Grand Prix", "Extra Grand Prix",
                                          "Time Attack"};
    SavedRaceProgress *groups[3];
    int i;

    groups[0] = &block->grandPrixProgress;
    groups[1] = &block->extraGrandPrixProgress;
    groups[2] = &block->timeAttackProgress;

    for (i = 0; i < 3; i++) {
        SavedRaceProgress *progress = groups[i];
        char money[24];
        char label[64];

        Heading(kNames[i]);
        igPushID_Int(i);

        {
            char preview[64];
            CarLabel(state, progress->carIndex, preview, sizeof(preview));
            igSetNextItemWidth(220.0f);
            if (igBeginCombo("car", preview, 0)) {
                int car;
                for (car = 0; car < 13; car++) {
                    char item[64];
                    CarLabel(state, car, item, sizeof(item));
                    if (igSelectable_Bool(item, progress->carIndex == car, 0,
                                          kAuto)) {
                        progress->carIndex = car;
                        state->dirty = 1;
                    }
                }
                igEndCombo();
            }
        }
        state->dirty |= WordField("course", &progress->course, 120.0f);
        igSameLine(0.0f, 20.0f);
        state->dirty |= WordField("class", &progress->classIndex, 120.0f);
        igSameLine(0.0f, 20.0f);
        state->dirty |=
            WordField("highest class", &progress->maxClassReached, 120.0f);

        state->dirty |= WordField("credits", &progress->money, 220.0f);
        igSameLine(0.0f, 12.0f);
        FormatMoney(progress->money, money, sizeof(money));
        igText("%s", money);
        igSameLine(0.0f, 20.0f);
        snprintf(label, sizeof(label), "fill up##money%d", i);
        if (igButton(label, kAuto)) {
            progress->money = 999999999;
            state->dirty = 1;
        }
        igPopID();
    }

    Heading("Unlocked");
    {
        bool unlocked = block->extraGrandPrixUnlocked != 0;
        if (igCheckbox("extra grand prix", &unlocked)) {
            block->extraGrandPrixUnlocked = unlocked ? 1 : 0;
            state->dirty = 1;
        }
        Explain("The game stores a number here rather than a flag; anything "
                "other than zero counts as unlocked.");
    }
    for (i = 0; i < 2; i++) {
        char label[64];
        snprintf(label, sizeof(label), "highest class reached %d", i + 1);
        state->dirty |= WordField(label, &block->maxClassReached[i], 120.0f);
        if (i == 0) igSameLine(0.0f, 20.0f);
    }

    Heading("Course progress");
    igTextDisabled("Four places, then whether an unlock is waiting and how "
                   "many retries are left.");
    if (igBeginTable("course progress", 7,
                     ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                         ImGuiTableFlags_SizingFixedFit,
                     kAuto, 0.0f)) {
        int row;
        igTableSetupColumn("series", 0, 150.0f, 0);
        for (i = 0; i < 4; i++) {
            char header[24];
            snprintf(header, sizeof(header), "place %d", i + 1);
            igTableSetupColumn(header, 0, 110.0f, 0);
        }
        igTableSetupColumn("unlock waiting", 0, 120.0f, 0);
        igTableSetupColumn("retries", 0, 110.0f, 0);
        igTableHeadersRow();
        for (row = 0; row < 2; row++) {
            unsigned char *bytes = row == 0
                                       ? block->grandPrixCourseProgress
                                       : block->extraGrandPrixCourseProgress;
            char id[32];
            /* The last four bytes are two little-endian halfwords. */
            int pending = bytes[4] | (bytes[5] << 8);
            int retries = bytes[6] | (bytes[7] << 8);
            bool waiting = pending != 0;

            igTableNextRow(0, 0.0f);
            igTableNextColumn();
            igText("%s", row == 0 ? "Grand Prix" : "Extra Grand Prix");
            for (i = 0; i < 4; i++) {
                igTableNextColumn();
                snprintf(id, sizeof(id), "##cp%d%d", row, i);
                state->dirty |= ByteSlider(id, &bytes[i], 255, 100.0f);
            }
            igTableNextColumn();
            snprintf(id, sizeof(id), "##pending%d", row);
            if (igCheckbox(id, &waiting)) {
                pending = waiting ? 1 : 0;
                bytes[4] = (unsigned char)(pending & 0xFF);
                bytes[5] = (unsigned char)((pending >> 8) & 0xFF);
                state->dirty = 1;
            }
            igTableNextColumn();
            snprintf(id, sizeof(id), "##retries%d", row);
            igSetNextItemWidth(100.0f);
            if (igInputInt(id, &retries, 0, 0, 0)) {
                bytes[6] = (unsigned char)(retries & 0xFF);
                bytes[7] = (unsigned char)((retries >> 8) & 0xFF);
                state->dirty = 1;
            }
        }
        igEndTable();
    }
}

/* ------------------------------------------------------------------ */

static void DrawGarage(EditorState *state) {
    GameSaveBlock *block = &state->save.block;
    static const char *const kNames[3] = {"Grand Prix", "Extra Grand Prix",
                                          "Time Attack"};
    static const char *const kGearbox[2] = {"automatic", "manual"};
    int garage;

    igTextDisabled("Three garages of thirteen cars.");
    igSameLine(0.0f, 16.0f);
    if (igButton("Give me every car, everywhere", kAuto)) {
        int i;
        int j;
        for (i = 0; i < 3; i++)
            for (j = 0; j < 13; j++) block->carSetup[i][j].enabled = 1;
        state->dirty = 1;
    }

    for (garage = 0; garage < 3; garage++) {
        char id[32];
        Heading(kNames[garage]);
        snprintf(id, sizeof(id), "garage%d", garage);
        if (!igBeginTable(id, 6,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_SizingFixedFit,
                          kAuto, 0.0f))
            continue;
        igTableSetupColumn("car", 0, 170.0f, 0);
        igTableSetupColumn("owned", 0, 55.0f, 0);
        igTableSetupColumn("grade", 0, 125.0f, 0);
        igTableSetupColumn("tyres", 0, 125.0f, 0);
        igTableSetupColumn("gearbox", 0, 125.0f, 0);
        igTableSetupColumn("paint", 0, 250.0f, 0);
        igTableHeadersRow();
        for (int car = 0; car < 13; car++) {
            SavedCarSetup *setup = &block->carSetup[garage][car];
            bool owned = setup->enabled != 0;
            char label[64];
            char id2[48];

            igTableNextRow(0, 0.0f);
            igTableNextColumn();
            CarLabel(state, car, label, sizeof(label));
            igText("%s", label);
            igTableNextColumn();
            snprintf(id2, sizeof(id2), "##own%d%d", garage, car);
            if (igCheckbox(id2, &owned)) {
                setup->enabled = owned ? 1 : 0;
                state->dirty = 1;
            }
            igTableNextColumn();
            snprintf(id2, sizeof(id2), "##grade%d%d", garage, car);
            state->dirty |= ByteSlider(id2, &setup->modelVariant, 4, 115.0f);
            igTableNextColumn();
            snprintf(id2, sizeof(id2), "##tyre%d%d", garage, car);
            state->dirty |= ByteSlider(id2, &setup->tireCompound, 4, 115.0f);
            igTableNextColumn();
            snprintf(id2, sizeof(id2), "##gear%d%d", garage, car);
            igSetNextItemWidth(115.0f);
            {
                int gearbox = setup->transmission != 0 ? 1 : 0;
                if (igBeginCombo(id2, kGearbox[gearbox], 0)) {
                    int option;
                    for (option = 0; option < 2; option++) {
                        if (igSelectable_Bool(kGearbox[option],
                                              gearbox == option, 0, kAuto)) {
                            setup->transmission = (unsigned char)option;
                            state->dirty = 1;
                        }
                    }
                    igEndCombo();
                }
            }
            igTableNextColumn();
            snprintf(id2, sizeof(id2), "##p1%d%d", garage, car);
            state->dirty |= ByteSlider(id2, &setup->paintColor1, 15, 115.0f);
            igSameLine(0.0f, 6.0f);
            snprintf(id2, sizeof(id2), "##p2%d%d", garage, car);
            state->dirty |= ByteSlider(id2, &setup->paintColor2, 15, 115.0f);
        }
        igEndTable();
    }
}

/* ------------------------------------------------------------------ */

static void DrawRecordSet(EditorState *state, const char *id,
                          RaceRecord records[4][5]) {
    int course;

    for (course = 0; course < 4; course++) {
        char title[64];
        char tableId[64];
        int place;

        snprintf(title, sizeof(title), "Course %d", course + 1);
        igSeparatorText(title);
        snprintf(tableId, sizeof(tableId), "%s_%d", id, course);
        if (!igBeginTable(tableId, 4,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_SizingFixedFit,
                          kAuto, 0.0f))
            continue;
        igTableSetupColumn("place", 0, 60.0f, 0);
        igTableSetupColumn("driver", 0, 150.0f, 0);
        igTableSetupColumn("time", 0, 160.0f, 0);
        igTableSetupColumn("car", 0, 220.0f, 0);
        igTableHeadersRow();
        for (place = 0; place < 5; place++) {
            RaceRecord *record = &records[course][place];
            char name[sizeof(record->driverName) + 1];
            char field[64];
            int milliseconds = record->raceTime;

            igTableNextRow(0, 0.0f);
            igTableNextColumn();
            igText("%d", place + 1);
            igTableNextColumn();
            memcpy(name, record->driverName, sizeof(record->driverName));
            name[sizeof(record->driverName)] = '\0';
            snprintf(field, sizeof(field), "##%s_n%d%d", id, course, place);
            igSetNextItemWidth(140.0f);
            if (igInputText(field, name, sizeof(name), 0, NULL, NULL)) {
                memset(record->driverName, 0, sizeof(record->driverName));
                memcpy(record->driverName, name,
                       strnlen(name, sizeof(record->driverName)));
                state->dirty = 1;
            }
            igTableNextColumn();
            snprintf(field, sizeof(field), "##%s_t%d%d", id, course, place);
            if (TimeField(field, &milliseconds, 150.0f)) {
                record->raceTime = milliseconds;
                state->dirty = 1;
            }
            igTableNextColumn();
            {
                char preview[64];
                CarLabel(state, record->carIndex, preview, sizeof(preview));
                snprintf(field, sizeof(field), "##%s_c%d%d", id, course, place);
                igSetNextItemWidth(200.0f);
                if (igBeginCombo(field, preview, 0)) {
                    int car;
                    for (car = 0; car < 13; car++) {
                        char item[64];
                        CarLabel(state, car, item, sizeof(item));
                        if (igSelectable_Bool(item, record->carIndex == car, 0,
                                              kAuto)) {
                            record->carIndex = (short)car;
                            state->dirty = 1;
                        }
                    }
                    igEndCombo();
                }
            }
        }
        igEndTable();
    }
}

static void DrawRecords(EditorState *state) {
    GameSaveBlock *block = &state->save.block;

    igTextDisabled("Times are written the way the game shows them, so 1'40\"765 "
                   "is what you type.");
    if (!igBeginTabBar("records", 0)) return;
    if (igBeginTabItem("Best times", NULL, 0)) {
        int set;
        for (set = 0; set < 2; set++) {
            char title[48];
            char id[32];
            int course;
            snprintf(title, sizeof(title), "Set %d", set + 1);
            Heading(title);
            snprintf(id, sizeof(id), "times%d", set);
            if (!igBeginTable(id, 8,
                              ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                                  ImGuiTableFlags_SizingFixedFit,
                              kAuto, 0.0f))
                continue;
            igTableSetupColumn("course", 0, 80.0f, 0);
            igTableSetupColumn("lap 1", 0, 160.0f, 0);
            igTableSetupColumn("lap 2", 0, 160.0f, 0);
            igTableSetupColumn("total 1", 0, 160.0f, 0);
            igTableSetupColumn("total 2", 0, 160.0f, 0);
            igTableSetupColumn("sector 1", 0, 160.0f, 0);
            igTableSetupColumn("sector 2", 0, 160.0f, 0);
            igTableSetupColumn("sector 3", 0, 160.0f, 0);
            igTableHeadersRow();
            for (course = 0; course < 4; course++) {
                char field[48];
                int slot;
                igTableNextRow(0, 0.0f);
                igTableNextColumn();
                igText("%d", course + 1);
                for (slot = 0; slot < 2; slot++) {
                    igTableNextColumn();
                    snprintf(field, sizeof(field), "##l%d%d%d", set, course,
                             slot);
                    state->dirty |= TimeField(
                        field, &block->bestLapTimes[set][course][slot], 150.0f);
                }
                for (slot = 0; slot < 2; slot++) {
                    igTableNextColumn();
                    snprintf(field, sizeof(field), "##T%d%d%d", set, course,
                             slot);
                    state->dirty |= TimeField(
                        field, &block->bestTotalTimes[set][course][slot],
                        150.0f);
                }
                for (slot = 0; slot < 3; slot++) {
                    igTableNextColumn();
                    snprintf(field, sizeof(field), "##s%d%d%d", set, course,
                             slot);
                    state->dirty |= TimeField(
                        field, &block->bestSectorTimes[set][course][slot],
                        150.0f);
                }
            }
            igEndTable();
        }
        igEndTabItem();
    }
    if (igBeginTabItem("Ranking", NULL, 0)) {
        int set;
        for (set = 0; set < 2; set++) {
            char title[48];
            char id[32];
            snprintf(title, sizeof(title), "Set %d", set + 1);
            Heading(title);
            snprintf(id, sizeof(id), "rank%d", set);
            DrawRecordSet(state, id, block->rankingRecords[set]);
        }
        igEndTabItem();
    }
    if (igBeginTabItem("Time attack", NULL, 0)) {
        int set;
        for (set = 0; set < 2; set++) {
            char title[48];
            char id[32];
            snprintf(title, sizeof(title), "Set %d", set + 1);
            Heading(title);
            snprintf(id, sizeof(id), "ta%d", set);
            DrawRecordSet(state, id, block->timeRecords[set]);
        }
        igEndTabItem();
    }
    if (igBeginTabItem("Class records", NULL, 0)) {
        int i;
        if (igBeginTable("classes", 3,
                         ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                             ImGuiTableFlags_SizingFixedFit,
                         kAuto, 0.0f)) {
            igTableSetupColumn("class", 0, 80.0f, 0);
            igTableSetupColumn("grade", 0, 160.0f, 0);
            igTableSetupColumn("clears", 0, 160.0f, 0);
            igTableHeadersRow();
            for (i = 0; i < 11; i++) {
                char field[32];
                igTableNextRow(0, 0.0f);
                igTableNextColumn();
                igText("%d", i + 1);
                igTableNextColumn();
                snprintf(field, sizeof(field), "##cg%d", i);
                state->dirty |=
                    HalfwordField(field, &block->classRecords[i].grade, 150.0f);
                igTableNextColumn();
                snprintf(field, sizeof(field), "##cc%d", i);
                state->dirty |= HalfwordField(
                    field, &block->classRecords[i].clears, 150.0f);
            }
            igEndTable();
        }
        igEndTabItem();
    }
    igEndTabBar();
}

/* ------------------------------------------------------------------ */

static void DrawLogo(EditorState *state) {
    GameSaveBlock *block = &state->save.block;
    ImDrawList *draw;
    ImVec2_c origin;
    float cell;
    int x;
    int y;
    int i;

    igTextDisabled("The logo painted on your car: 64 by 64, sixteen colours.");
    igSetNextItemWidth(220.0f);
    igSliderInt("zoom", &state->logoZoom, 3, 14, "%d", 0);
    igSameLine(0.0f, 24.0f);
    if (igButton("Wipe the canvas", kAuto)) {
        memset(block->teamLogoCanvas, 0, sizeof(block->teamLogoCanvas));
        state->dirty = 1;
    }

    Heading("Colours");
    igTextDisabled("Click a swatch to change it, the circle beside it to paint "
                   "with it.");
    for (i = 0; i < 16; i++) {
        unsigned char rgb[3];
        int transparent;
        float colour[3];
        char label[40];

        RageLogoColour(block->teamLogoClut[i], rgb, &transparent);
        colour[0] = rgb[0] / 255.0f;
        colour[1] = rgb[1] / 255.0f;
        colour[2] = rgb[2] / 255.0f;

        igBeginGroup();
        snprintf(label, sizeof(label), "##clut%d", i);
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
        igSameLine(0.0f, 4.0f);
        snprintf(label, sizeof(label), "##pick%d", i);
        if (igRadioButton_Bool(label, state->logoColour == i))
            state->logoColour = i;
        igEndGroup();
        if ((i % 8) != 7) igSameLine(0.0f, 10.0f);
    }
    {
        unsigned char rgb[3];
        int transparent;
        bool clear;
        RageLogoColour(block->teamLogoClut[state->logoColour], rgb,
                       &transparent);
        clear = transparent != 0;
        if (igCheckbox("this colour is see through", &clear)) {
            block->teamLogoClut[state->logoColour] =
                RageLogoPackColour(rgb, clear ? 1 : 0);
            state->dirty = 1;
        }
        Explain("A see-through entry is drawn as a chequer here so it cannot "
                "be mistaken for black.");
    }

    Heading("Canvas");
    igTextDisabled("Drag with the left button to paint, right button to pick a "
                   "colour up.");
    cell = (float)state->logoZoom;
    origin = igGetCursorScreenPos();
    draw = igGetWindowDrawList();
    for (y = 0; y < RAGE_LOGO_HEIGHT; y++) {
        for (x = 0; x < RAGE_LOGO_WIDTH; x++) {
            unsigned char rgb[3];
            int transparent;
            unsigned int packed;

            RageLogoColour(block->teamLogoClut[RageLogoPixel(block, x, y)], rgb,
                           &transparent);
            if (transparent)
                packed = ((x + y) & 1) ? 0xFF3A3A3Au : 0xFF2A2A2Au;
            else
                packed = 0xFF000000u | ((unsigned int)rgb[2] << 16) |
                         ((unsigned int)rgb[1] << 8) | rgb[0];
            ImDrawList_AddRectFilled(draw, Size(origin.x + x * cell,
                                                origin.y + y * cell),
                                     Size(origin.x + (x + 1) * cell,
                                          origin.y + (y + 1) * cell),
                                     packed, 0.0f, 0);
        }
    }
    ImDrawList_AddRect(draw, origin,
                       Size(origin.x + RAGE_LOGO_WIDTH * cell,
                            origin.y + RAGE_LOGO_HEIGHT * cell),
                       0xFF808080u, 0.0f, 0, 1.0f);
    igInvisibleButton("canvas",
                      Size(RAGE_LOGO_WIDTH * cell, RAGE_LOGO_HEIGHT * cell), 0);
    if (igIsItemHovered(0)) {
        ImVec2_c mouse = igGetMousePos();
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

/* ------------------------------------------------------------------ */

static void DrawSettings(EditorState *state) {
    GameSaveBlock *block = &state->save.block;

    Heading("Sound");
    {
        int track = block->bgmSelection;
        igSetNextItemWidth(220.0f);
        if (igInputInt("music track", &track, 1, 1, 0)) {
            block->bgmSelection = (short)track;
            state->dirty = 1;
        }
        state->dirty |= WordField("music volume", &block->bgmVolume, 220.0f);
        state->dirty |= WordField("effects volume", &block->sfxVolume, 220.0f);
        {
            bool mono = block->monoOutput != 0;
            if (igCheckbox("mono", &mono)) {
                block->monoOutput = mono ? 1 : 0;
                state->dirty = 1;
            }
        }
    }

    Heading("Controller");
    state->dirty |= HalfwordField("pad layout", &block->padMappingIndex, 160.0f);
    state->dirty |=
        HalfwordField("neGcon layout", &block->negconMappingIndex, 160.0f);

    Heading("neGcon calibration");
    igTextDisabled("Set by the game when you calibrate the controller.");
    state->dirty |=
        HalfwordField("steering centre", &block->negconSteerNeutral, 160.0f);
    igSameLine(0.0f, 20.0f);
    state->dirty |=
        HalfwordField("steering play", &block->negconSteerPlay, 160.0f);
    state->dirty |= HalfwordField("I centre", &block->negconNeutralI, 160.0f);
    igSameLine(0.0f, 20.0f);
    state->dirty |= HalfwordField("II centre", &block->negconNeutralII, 160.0f);
    igSameLine(0.0f, 20.0f);
    state->dirty |= HalfwordField("L centre", &block->negconNeutralL, 160.0f);
    state->dirty |= HalfwordField("full twist", &block->negconMaxTwist, 160.0f);
}

/* ------------------------------------------------------------------ */

static void DrawFile(EditorState *state) {
    const RageRegionInfo *regions;
    size_t regionCount;
    size_t i;
    char name[64];
    char team[RAGE_TEAM_NAME_LENGTH + 1];

    Heading("Team");
    RageSaveReadTeamName(&state->save.header, team, sizeof(team));
    igSetNextItemWidth(240.0f);
    if (igInputText("name", team, sizeof(team), 0, NULL, NULL)) {
        RageSaveWriteTeamName(&state->save.header, team);
        state->dirty = 1;
    }
    igTextDisabled("Up to seven letters, digits, or  . - ! ? @");

    Heading("Which release");
    regions = RageRegionTable(&regionCount);
    for (i = 0; i < regionCount; i++) {
        char label[96];
        snprintf(label, sizeof(label), "%s   %s", regions[i].name,
                 regions[i].serial);
        if (igRadioButton_Bool(label, state->region == regions[i].region)) {
            state->region = regions[i].region;
            state->dirty = 1;
        }
        if (!regions[i].verified)
            Explain("This disc's serial is not recorded in the port, so the "
                    "file name below may need correcting by hand.");
        if (i + 1 < regionCount) igSameLine(0.0f, 24.0f);
    }
    igTextDisabled("All three keep the same fields. Only the name the file "
                   "needs on a memory card changes.");

    Heading("Slot");
    igSetNextItemWidth(300.0f);
    if (igSliderInt("slot", &state->slot, 0, RAGE_SAVE_SLOTS - 1, "%d", 0))
        state->dirty = 1;
    EditorSuggestedName(state, name, sizeof(name));
    igText("File name on a card:   %s", name);

    Heading("Condition");
    if (state->report.headerChecksumValid && state->report.blockChecksumValid &&
        state->report.trailerChecksumValid && state->report.trailerMatchesHeader)
        igText("This save is sound.");
    else
        igText("This save arrived damaged. Saving it repairs it.");
    Explain("A save carries three checksums. They are recomputed every time "
            "this program writes a file, so a save the game refused becomes "
            "one it accepts.");
    igTextDisabled("card signature %s   header %s   payload %s   copy %s",
                   state->report.iconRecognised ? "ok" : "missing",
                   state->report.headerChecksumValid ? "ok" : "wrong",
                   state->report.blockChecksumValid ? "ok" : "wrong",
                   state->report.trailerMatchesHeader ? "ok" : "differs");
}

/* ------------------------------------------------------------------ */

static void DrawAdvanced(EditorState *state) {
    unsigned int i;

    igTextDisabled("Nothing here needs changing to play. It is shown so that a "
                   "file edited by this program keeps everything it arrived "
                   "with.");

    Heading("Save counter");
    {
        int counter = state->save.header.fields.saveCounter;
        if (WordField("counter", &counter, 220.0f)) {
            state->save.header.fields.saveCounter = counter;
            state->dirty = 1;
        }
    }

    Heading("Bytes the game never reads");
    for (i = 0; i < sizeof(state->save.block.reserved); i++) {
        char label[32];
        snprintf(label, sizeof(label), "##r%u", i);
        state->dirty |= ByteSlider(label, &state->save.block.reserved[i], 255,
                                   90.0f);
        if ((i % 8) != 7) igSameLine(0.0f, 6.0f);
    }

    Heading("Icon block");
    igTextDisabled("The first 0x200 bytes: the card signature, the title the "
                   "console shows and the icon. Kept exactly as found.");
    igText("signature %c%c    frames %02X    blocks %02X", state->save.icon[0],
           state->save.icon[1], state->save.icon[2], state->save.icon[3]);
}

/* ------------------------------------------------------------------ */

static const char *const kSectionNames[EDITOR_SECTION_COUNT] = {
    "Progress", "Garage", "Records", "Team logo", "Settings", "File", "Advanced"
};

void EditorDrawWindow(EditorState *state, EditorRequests *requests) {
    if (state->openTab != NULL) {
        int i;
        for (i = 0; i < (int)EDITOR_SECTION_COUNT; i++) {
            if (strcmp(state->openTab, kSectionNames[i]) == 0)
                state->section = (EditorSection)i;
        }
    }

    if (!state->loaded) {
        DrawWelcome(state, requests);
        return;
    }

    /* Header: what is open, and what it is. */
    {
        const RageRegionInfo *info = RageRegionFind(state->region);
        const char *name = state->path[0] != '\0' ? state->path : "new save";
        igPushFont(NULL, igGetFontSize() * 1.25f);
        igText("%s", RageSaveSlotFromPath(name) >= 0 || state->path[0] != '\0'
                         ? strrchr(name, '/') ? strrchr(name, '/') + 1 : name
                         : "New save");
        igPopFont();
        igTextDisabled("%s   slot %d%s", info != NULL ? info->name : "unknown",
                       state->slot, state->dirty ? "   unsaved changes" : "");
    }
    igSeparator();

    if (igBeginChild_Str("sections", Size(190.0f, -44.0f), 0, 0)) {
        int i;
        for (i = 0; i < EDITOR_SECTION_COUNT; i++) {
            if (igSelectable_Bool(kSectionNames[i], state->section == (EditorSection)i, 0,
                                  Size(0.0f, 30.0f)))
                state->section = (EditorSection)i;
        }
    }
    igEndChild();
    igSameLine(0.0f, 16.0f);
    if (igBeginChild_Str("panel", Size(0.0f, -44.0f), 0, 0)) {
        switch (state->section) {
        case EDITOR_SECTION_PROGRESS: DrawProgress(state); break;
        case EDITOR_SECTION_GARAGE: DrawGarage(state); break;
        case EDITOR_SECTION_RECORDS: DrawRecords(state); break;
        case EDITOR_SECTION_LOGO: DrawLogo(state); break;
        case EDITOR_SECTION_SETTINGS: DrawSettings(state); break;
        case EDITOR_SECTION_FILE: DrawFile(state); break;
        case EDITOR_SECTION_ADVANCED: DrawAdvanced(state); break;
        default: break;
        }
    }
    igEndChild();

    /* Footer: the things a person came here to do. */
    igSeparator();
    if (igButton("Save", Size(110.0f, 32.0f))) {
        if (state->path[0] != '\0')
            EditorSave(state, state->path);
        else
            requests->saveAs = 1;
    }
    igSameLine(0.0f, 10.0f);
    if (igButton("Save as...", Size(130.0f, 32.0f))) requests->saveAs = 1;
    igSameLine(0.0f, 10.0f);
    if (igButton("Open...", Size(110.0f, 32.0f))) requests->open = 1;
    igSameLine(0.0f, 10.0f);
    if (igButton("Close", Size(110.0f, 32.0f))) {
        state->loaded = 0;
        state->dirty = 0;
        state->path[0] = '\0';
        EditorRescan(state);
    }
    igSameLine(0.0f, 20.0f);
    if (state->statusIsError)
        igTextColored((ImVec4_c){1.0f, 0.45f, 0.4f, 1.0f}, "%s", state->status);
    else
        igTextDisabled("%s", state->status);
}

void EditorSuggestedName(const EditorState *state, char *out, size_t size) {
    if (!RageRegionCardName(state->region, state->slot, out, size))
        snprintf(out, size, "RAGE%03d", state->slot);
}

void EditorRescan(EditorState *state) {
    state->foundCount = RageSaveDiscover(state->found, RAGE_SAVE_DISCOVER_MAX);
    state->scanned = 1;
}

void EditorOpen(EditorState *state, const char *path) {
    if (RageSaveLoad(path, &state->save, &state->report)) {
        int slot = RageSaveSlotFromPath(path);
        snprintf(state->path, sizeof(state->path), "%s", path);
        state->loaded = 1;
        state->dirty = 0;
        state->statusIsError = 0;
        if (state->report.region != RAGE_REGION_UNKNOWN)
            state->region = state->report.region;
        if (slot >= 0 && slot < RAGE_SAVE_SLOTS) state->slot = slot;
        snprintf(state->status, sizeof(state->status), "opened");
    } else {
        state->loaded = 0;
        state->statusIsError = 1;
        snprintf(state->status, sizeof(state->status), "%s",
                 state->report.detail);
    }
}

void EditorSave(EditorState *state, const char *path) {
    if (RageSaveStore(path, &state->save, &state->report)) {
        snprintf(state->path, sizeof(state->path), "%s", path);
        state->dirty = 0;
        state->statusIsError = 0;
        snprintf(state->status, sizeof(state->status), "saved");
        EditorRescan(state);
    } else {
        state->statusIsError = 1;
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
    state->statusIsError = 0;
    state->path[0] = '\0';
    state->section = EDITOR_SECTION_FILE;
    snprintf(state->status, sizeof(state->status), "not saved yet");
}
