#include "car_catalog.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/asset.h"
#include "game/menu.h"
#include "game/race.h"

static const unsigned char kFirstVariant[GAME_CAR_COUNT] = {
    0, 4, 7, 9, 14, 18, 21, 23, 26, 28, 29, 30, 31
};
static RageCarCatalog s_Catalog;
static int s_CatalogLoaded;

static int Fail(char *error, size_t size, const char *format, ...) {
    va_list args;
    if (error != NULL && size != 0) {
        va_start(args, format);
        vsnprintf(error, size, format, args);
        va_end(args);
    }
    return 0;
}

int CarCatalogValidate(const RageCarCatalog *catalog, char *error,
                       size_t errorSize) {
    unsigned char seen[RAGE_CAR_CATALOG_ENTRY_COUNT] = {0};
    size_t index;
    if (catalog == NULL || catalog->count == 0)
        return Fail(error, errorSize, "catalog must contain at least one variant");
    for (index = 0; index < catalog->count; index++) {
        const RageCarCatalogEntry *entry = &catalog->entries[index];
        int first, end, variant;
        if (entry->modelIndex < 0 || entry->modelIndex >= GAME_CAR_COUNT)
            return Fail(error, errorSize, "entry %zu has invalid model", index);
        first = kFirstVariant[entry->modelIndex];
        end = entry->modelIndex + 1 < GAME_CAR_COUNT
                  ? kFirstVariant[entry->modelIndex + 1]
                  : CAR_MODEL_VARIANT_COUNT;
        variant = first + entry->grade;
        if (entry->grade < 0 || variant >= end || seen[variant])
            return Fail(error, errorSize, "entry %zu has invalid or duplicate grade", index);
        if ((entry->fields & (RAGE_CAR_FIELD_ID | RAGE_CAR_FIELD_MODEL |
                              RAGE_CAR_FIELD_GRADE)) !=
            (RAGE_CAR_FIELD_ID | RAGE_CAR_FIELD_MODEL | RAGE_CAR_FIELD_GRADE) ||
            !entry->id[0] ||
            ((entry->fields & RAGE_CAR_FIELD_PRICE) && entry->price < 0) ||
            ((entry->fields & RAGE_CAR_FIELD_UPGRADE_PRICE) && entry->upgradePrice < 0) ||
            ((entry->fields & RAGE_CAR_FIELD_UNLOCK_CLASS) && entry->unlockClass < 0) ||
            ((entry->fields & RAGE_CAR_FIELD_MANUAL_ONLY) &&
             (entry->manualOnly < 0 || entry->manualOnly > 1)) ||
            ((entry->fields & RAGE_CAR_FIELD_REV_LIMIT) &&
             entry->specification.revLimit <= 0) ||
            ((entry->fields & RAGE_CAR_FIELD_AUTOMATIC_ACCELERATION_SCALE) &&
             entry->specification.automaticAccelerationScale <= 0) ||
            ((entry->fields & RAGE_CAR_FIELD_TOP_GEAR) &&
             (entry->specification.topGear < 1 || entry->specification.topGear > 6)))
            return Fail(error, errorSize, "entry %zu has invalid override metadata", index);
        seen[variant] = 1;
    }
    if (error != NULL && errorSize != 0) error[0] = '\0';
    return 1;
}

static char *Trim(char *text) {
    char *end;
    while (*text == ' ' || *text == '\t' || *text == '\r') text++;
    end = text + strlen(text);
    while (end > text && (end[-1] == ' ' || end[-1] == '\t' ||
                          end[-1] == '\r')) *--end = '\0';
    return text;
}

static int CopyString(char *out, size_t outSize, const char *value) {
    size_t length;
    if (value == NULL || value[0] != '"') return 0;
    value++;
    length = strlen(value);
    if (length == 0 || value[length - 1] != '"') return 0;
    length--;
    if (length >= outSize || strchr(value, '\\') != NULL) return 0;
    memcpy(out, value, length);
    out[length] = '\0';
    return 1;
}

static int ParseInt(const char *value, int *out) {
    char *end;
    long number;
    if (value == NULL || *value == '\0') return 0;
    number = strtol(value, &end, 10);
    if (*end != '\0' || number < -2147483647L - 1 || number > 2147483647L)
        return 0;
    *out = (int)number;
    return 1;
}

static int ParseBool(const char *value, int *out) {
    if (strcmp(value, "true") == 0) { *out = 1; return 1; }
    if (strcmp(value, "false") == 0) { *out = 0; return 1; }
    return 0;
}

static int ParseIntArray(const char *value, s32 *out, size_t count) {
    size_t index = 0;
    const char *cursor = value;
    if (*cursor++ != '[') return 0;
    while (index < count) {
        char *end;
        long number;
        while (*cursor == ' ' || *cursor == '\t') cursor++;
        number = strtol(cursor, &end, 10);
        if (end == cursor || number < -2147483647L - 1 || number > 2147483647L)
            return 0;
        out[index++] = (s32)number;
        cursor = end;
        while (*cursor == ' ' || *cursor == '\t') cursor++;
        if (index == count) break;
        if (*cursor++ != ',') return 0;
    }
    while (*cursor == ' ' || *cursor == '\t') cursor++;
    return *cursor++ == ']' && *cursor == '\0';
}

static int ParseShiftPoints(const char *value, GameCarSpecShiftPoint *out) {
    s32 values[CAR_FORWARD_GEAR_COUNT * 2];
    size_t index;
    if (!ParseIntArray(value, values, CAR_FORWARD_GEAR_COUNT * 2)) return 0;
    for (index = 0; index < CAR_FORWARD_GEAR_COUNT; index++) {
        if (values[index * 2] < -32768 || values[index * 2] > 32767 ||
            values[index * 2 + 1] < -32768 || values[index * 2 + 1] > 32767)
            return 0;
        out[index].downshiftSpeed = (s16)values[index * 2];
        out[index].upshiftSpeed = (s16)values[index * 2 + 1];
    }
    return 1;
}

static int ParseTorqueScale(const char *value, s16 *out) {
    s32 values[6];
    size_t index;
    if (!ParseIntArray(value, values, 6)) return 0;
    for (index = 0; index < 6; index++) {
        if (values[index] < -32768 || values[index] > 32767) return 0;
        out[index] = (s16)values[index];
    }
    return 1;
}

static int SetEntryValue(RageCarCatalogEntry *entry, const char *key,
                         const char *value) {
    int integer;
#define SET_STRING(name, member, bit) if (strcmp(key, name) == 0) { if ((entry->fields & bit) || !CopyString(member, sizeof(member), value)) return 0; entry->fields |= bit; return 1; }
    SET_STRING("id", entry->id, RAGE_CAR_FIELD_ID)
    SET_STRING("name", entry->name, RAGE_CAR_FIELD_NAME)
    SET_STRING("manufacturer", entry->manufacturer, RAGE_CAR_FIELD_MANUFACTURER)
    SET_STRING("class", entry->className, RAGE_CAR_FIELD_CLASS)
#undef SET_STRING
#define SET_INT(name, member, bit) if (strcmp(key, name) == 0) { if ((entry->fields & bit) || !ParseInt(value, &integer)) return 0; entry->member = integer; entry->fields |= bit; return 1; }
    SET_INT("model", modelIndex, RAGE_CAR_FIELD_MODEL)
    SET_INT("grade", grade, RAGE_CAR_FIELD_GRADE)
    SET_INT("price", price, RAGE_CAR_FIELD_PRICE)
    SET_INT("upgrade_price", upgradePrice, RAGE_CAR_FIELD_UPGRADE_PRICE)
    SET_INT("unlock_class", unlockClass, RAGE_CAR_FIELD_UNLOCK_CLASS)
#undef SET_INT
#define SET_VALUE(name, bit, expression) if (strcmp(key, name) == 0) { if ((entry->fields & bit) || !(expression)) return 0; entry->fields |= bit; return 1; }
    SET_VALUE("manual_only", RAGE_CAR_FIELD_MANUAL_ONLY, ParseBool(value, &entry->manualOnly))
    SET_VALUE("torque_curve", RAGE_CAR_FIELD_TORQUE_CURVE, ParseIntArray(value, entry->specification.torqueCurve, 16))
    SET_VALUE("torque_band", RAGE_CAR_FIELD_TORQUE_BAND, ParseIntArray(value, entry->specification.torqueBand.values, 16))
    SET_VALUE("torque_loss_value", RAGE_CAR_FIELD_TORQUE_LOSS_VALUE, ParseIntArray(value, entry->specification.torqueLossValue, 10))
    SET_VALUE("torque_loss_rpm", RAGE_CAR_FIELD_TORQUE_LOSS_RPM, ParseIntArray(value, entry->specification.torqueLossRpm, 9))
    SET_VALUE("gear_load", RAGE_CAR_FIELD_GEAR_LOAD, ParseIntArray(value, entry->specification.gearLoad, 6))
    SET_VALUE("gear_ratio", RAGE_CAR_FIELD_GEAR_RATIO, ParseIntArray(value, entry->specification.gearRatio, 7))
    SET_VALUE("torque_scale", RAGE_CAR_FIELD_TORQUE_SCALE, ParseTorqueScale(value, entry->specification.torqueScale))
    SET_VALUE("shift_points", RAGE_CAR_FIELD_SHIFT_POINTS, ParseShiftPoints(value, entry->specification.shiftPoints))
#define SET_SCALAR(name, member, bit) if (strcmp(key, name) == 0) { if ((entry->fields & bit) || !ParseInt(value, &integer) || integer < -32768 || integer > 32767) return 0; entry->specification.member = (s16)integer; entry->fields |= bit; return 1; }
    SET_SCALAR("rev_limit", revLimit, RAGE_CAR_FIELD_REV_LIMIT)
    SET_SCALAR("automatic_acceleration_scale", automaticAccelerationScale, RAGE_CAR_FIELD_AUTOMATIC_ACCELERATION_SCALE)
    SET_SCALAR("top_gear", topGear, RAGE_CAR_FIELD_TOP_GEAR)
    SET_SCALAR("redline", redline, RAGE_CAR_FIELD_REDLINE)
    SET_SCALAR("steering_grip_response", steeringGripResponse, RAGE_CAR_FIELD_STEERING_GRIP_RESPONSE)
    SET_SCALAR("reference_turn_radius", referenceTurnRadius, RAGE_CAR_FIELD_REFERENCE_TURN_RADIUS)
    SET_SCALAR("negcon_steering_assist_scale", negconSteeringAssistScale, RAGE_CAR_FIELD_NEGCON_STEERING_ASSIST_SCALE)
    SET_SCALAR("speed_drag_divisor", speedDragDivisor, RAGE_CAR_FIELD_SPEED_DRAG_DIVISOR)
    SET_SCALAR("base_steering_grip", baseSteeringGrip, RAGE_CAR_FIELD_BASE_STEERING_GRIP)
#undef SET_SCALAR
    if (strcmp(key, "steer_response") == 0) { if ((entry->fields & RAGE_CAR_FIELD_STEER_RESPONSE) || !ParseInt(value, &integer) || integer < 0 || integer > 65535) return 0; entry->specification.steerResponse = (u16)integer; entry->fields |= RAGE_CAR_FIELD_STEER_RESPONSE; return 1; }
#undef SET_VALUE
    return 0;
}

int CarCatalogParse(const char *text, RageCarCatalog *catalog,
                    char *error, size_t errorSize) {
    char *copy, *line, *next;
    RageCarCatalogEntry *entry = NULL;
    if (text == NULL || catalog == NULL) return Fail(error, errorSize, "catalog text is missing");
    memset(catalog, 0, sizeof(*catalog));
    copy = malloc(strlen(text) + 1);
    if (copy == NULL) return Fail(error, errorSize, "out of memory parsing catalog");
    strcpy(copy, text);
    for (line = copy; line != NULL; line = next) {
        char *equals, *key, *value, *comment;
        next = strchr(line, '\n'); if (next != NULL) *next++ = '\0';
        comment = strchr(line, '#'); if (comment != NULL) *comment = '\0';
        line = Trim(line); if (*line == '\0') continue;
        if (strcmp(line, "[[cars]]") == 0) {
            if (catalog->count == RAGE_CAR_CATALOG_ENTRY_COUNT) { free(copy); return Fail(error,errorSize,"too many cars"); }
            entry = &catalog->entries[catalog->count++]; memset(entry, 0, sizeof(*entry));
            continue;
        }
        equals = strchr(line, '=');
        if (entry == NULL || equals == NULL) { free(copy); return Fail(error,errorSize,"invalid catalog line"); }
        *equals = '\0'; key = Trim(line); value = Trim(equals + 1);
        if (!SetEntryValue(entry, key, value)) { free(copy); return Fail(error,errorSize,"invalid value for %s", key); }
    }
    free(copy);
    return CarCatalogValidate(catalog, error, errorSize);
}

int CarCatalogLoadFile(const char *path, char *error, size_t errorSize) {
    FILE *file; long size; char *text; RageCarCatalog next, ordered;
    size_t index;
    if (path == NULL || (file = fopen(path, "rb")) == NULL) return Fail(error,errorSize,"cannot open car catalog");
    if (fseek(file, 0, SEEK_END) != 0 || (size = ftell(file)) < 0 || fseek(file, 0, SEEK_SET) != 0 || size > 1024 * 1024) { fclose(file); return Fail(error,errorSize,"invalid car catalog size"); }
    text = malloc((size_t)size + 1); if (text == NULL) { fclose(file); return Fail(error,errorSize,"out of memory reading catalog"); }
    if (fread(text, 1, (size_t)size, file) != (size_t)size || fclose(file) != 0) { free(text); return Fail(error,errorSize,"cannot read car catalog"); }
    text[size] = '\0';
    if (!CarCatalogParse(text, &next, error, errorSize)) { free(text); return 0; }
    free(text);
    memset(&ordered, 0, sizeof(ordered));
    ordered.count = next.count;
    for (index = 0; index < next.count; index++) {
        RageCarCatalogEntry *entry = &next.entries[index];
        ordered.entries[kFirstVariant[entry->modelIndex] + entry->grade] = *entry;
    }
    s_Catalog = ordered; s_CatalogLoaded = 1; return 1;
}

void CarCatalogClearOverrides(void) {
    memset(&s_Catalog, 0, sizeof(s_Catalog));
    s_CatalogLoaded = 0;
}

static RageCarCatalogEntry *FindEntry(int modelIndex, int grade) {
    int variant;
    if (!s_CatalogLoaded || modelIndex < 0 || modelIndex >= GAME_CAR_COUNT || grade < 0) return NULL;
    variant = kFirstVariant[modelIndex] + grade;
    if (variant < 0 || variant >= CAR_MODEL_VARIANT_COUNT) return NULL;
    return &s_Catalog.entries[variant];
}

void CarCatalogApplyMetadata(void) {
    size_t index;
    if (!s_CatalogLoaded) return;
    for (index = 0; index < CAR_MODEL_VARIANT_COUNT; index++) {
        RageCarCatalogEntry *entry = &s_Catalog.entries[index];
        if (entry->fields & RAGE_CAR_FIELD_PRICE) g_CarPriceTable[index] = entry->price;
        if ((entry->fields & RAGE_CAR_FIELD_UPGRADE_PRICE) &&
            index < CAR_MODEL_VARIANT_COUNT - 1) g_CarTuneUpPriceTable[index] = entry->upgradePrice;
        if (entry->grade == 0) {
            if (entry->fields & RAGE_CAR_FIELD_NAME) g_NativeCarNames[entry->modelIndex] = entry->name;
            if (entry->fields & RAGE_CAR_FIELD_MANUFACTURER) g_NativeCarManufacturerNames[entry->modelIndex] = entry->manufacturer;
            if (entry->fields & RAGE_CAR_FIELD_CLASS) g_NativeCarClassNames[entry->modelIndex] = entry->className;
        }
    }
}

void CarCatalogApplySpecification(int modelIndex, int grade, GameCarSpec *specification) {
    RageCarCatalogEntry *entry = FindEntry(modelIndex, grade);
    if (entry == NULL || specification == NULL) return;
#define COPY_ARRAY(bit, member) if (entry->fields & bit) memcpy(specification->member, entry->specification.member, sizeof(specification->member))
    COPY_ARRAY(RAGE_CAR_FIELD_TORQUE_CURVE, torqueCurve);
    COPY_ARRAY(RAGE_CAR_FIELD_TORQUE_BAND, torqueBand.values);
    COPY_ARRAY(RAGE_CAR_FIELD_TORQUE_LOSS_VALUE, torqueLossValue);
    COPY_ARRAY(RAGE_CAR_FIELD_TORQUE_LOSS_RPM, torqueLossRpm);
    COPY_ARRAY(RAGE_CAR_FIELD_GEAR_LOAD, gearLoad);
    COPY_ARRAY(RAGE_CAR_FIELD_GEAR_RATIO, gearRatio);
    COPY_ARRAY(RAGE_CAR_FIELD_TORQUE_SCALE, torqueScale);
    COPY_ARRAY(RAGE_CAR_FIELD_SHIFT_POINTS, shiftPoints);
#undef COPY_ARRAY
#define COPY_SCALAR(bit, member) if (entry->fields & bit) specification->member = entry->specification.member
    COPY_SCALAR(RAGE_CAR_FIELD_REV_LIMIT, revLimit);
    COPY_SCALAR(RAGE_CAR_FIELD_AUTOMATIC_ACCELERATION_SCALE, automaticAccelerationScale);
    COPY_SCALAR(RAGE_CAR_FIELD_TOP_GEAR, topGear);
    COPY_SCALAR(RAGE_CAR_FIELD_REDLINE, redline);
    COPY_SCALAR(RAGE_CAR_FIELD_STEERING_GRIP_RESPONSE, steeringGripResponse);
    COPY_SCALAR(RAGE_CAR_FIELD_STEER_RESPONSE, steerResponse);
    COPY_SCALAR(RAGE_CAR_FIELD_REFERENCE_TURN_RADIUS, referenceTurnRadius);
    COPY_SCALAR(RAGE_CAR_FIELD_NEGCON_STEERING_ASSIST_SCALE, negconSteeringAssistScale);
    COPY_SCALAR(RAGE_CAR_FIELD_SPEED_DRAG_DIVISOR, speedDragDivisor);
    COPY_SCALAR(RAGE_CAR_FIELD_BASE_STEERING_GRIP, baseSteeringGrip);
#undef COPY_SCALAR
}

void CarCatalogApplyModelAvailability(int modelIndex, int grade, struct CarModelAsset *asset) {
    RageCarCatalogEntry *entry = FindEntry(modelIndex, grade);
    if (entry == NULL) return;
    if (!(entry->fields & RAGE_CAR_FIELD_MANUAL_ONLY)) return;
    if (asset != NULL) asset->transmissionAvailable = (u8)!entry->manualOnly;
    /* A pre-existing modded save can request automatic even though the menu
     * correctly hides that row.  Keep the runtime state consistent with the
     * catalog when this model is loaded. */
    if (entry->manualOnly && g_CarTable != NULL &&
        (unsigned)modelIndex < GAME_CAR_COUNT) {
        g_CarTable[modelIndex].transmission = 1;
    }
}

int CarCatalogUnlockClass(int modelIndex, int grade, int fallback) {
    RageCarCatalogEntry *entry = FindEntry(modelIndex, grade);
    return entry != NULL && (entry->fields & RAGE_CAR_FIELD_UNLOCK_CLASS)
               ? entry->unlockClass : fallback;
}
