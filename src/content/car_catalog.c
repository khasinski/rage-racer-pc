#include "port/car_catalog.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static const unsigned char kFirstVariant[GAME_CAR_COUNT] = {
    0, 4, 7, 9, 14, 18, 21, 23, 26, 28, 29, 30, 31
};

#define FIELD(name,member,bit,type,count) {name,bit,offsetof(RageCarCatalogEntry,member),count,type}
static const CarField fields[] = {
    FIELD("id",id,RAGE_CAR_FIELD_ID,FIELD_TEXT,32),
    FIELD("model",modelIndex,RAGE_CAR_FIELD_MODEL,FIELD_INT,1),
    FIELD("grade",grade,RAGE_CAR_FIELD_GRADE,FIELD_INT,1),
    FIELD("name",name,RAGE_CAR_FIELD_NAME,FIELD_TEXT,32),
    FIELD("manufacturer",manufacturer,RAGE_CAR_FIELD_MANUFACTURER,FIELD_TEXT,32),
    FIELD("class",className,RAGE_CAR_FIELD_CLASS,FIELD_TEXT,32),
    FIELD("price",price,RAGE_CAR_FIELD_PRICE,FIELD_INT,1),
    FIELD("upgrade_price",upgradePrice,RAGE_CAR_FIELD_UPGRADE_PRICE,FIELD_INT,1),
    FIELD("unlock_class",unlockClass,RAGE_CAR_FIELD_UNLOCK_CLASS,FIELD_INT,1),
    FIELD("manual_only",manualOnly,RAGE_CAR_FIELD_MANUAL_ONLY,FIELD_BOOL,1),
    FIELD("torque_curve",specification.torqueCurve,RAGE_CAR_FIELD_TORQUE_CURVE,FIELD_INT,16),
    FIELD("torque_band",specification.torqueBand.values,RAGE_CAR_FIELD_TORQUE_BAND,FIELD_INT,16),
    FIELD("torque_loss_value",specification.torqueLossValue,RAGE_CAR_FIELD_TORQUE_LOSS_VALUE,FIELD_INT,10),
    FIELD("torque_loss_rpm",specification.torqueLossRpm,RAGE_CAR_FIELD_TORQUE_LOSS_RPM,FIELD_INT,9),
    FIELD("gear_load",specification.gearLoad,RAGE_CAR_FIELD_GEAR_LOAD,FIELD_INT,6),
    FIELD("gear_ratio",specification.gearRatio,RAGE_CAR_FIELD_GEAR_RATIO,FIELD_INT,7),
    FIELD("torque_scale",specification.torqueScale,RAGE_CAR_FIELD_TORQUE_SCALE,FIELD_SHORT,6),
    FIELD("shift_points",specification.shiftPoints,RAGE_CAR_FIELD_SHIFT_POINTS,FIELD_SHORT,12),
    FIELD("rev_limit",specification.revLimit,RAGE_CAR_FIELD_REV_LIMIT,FIELD_SHORT,1),
    FIELD("automatic_acceleration_scale",specification.automaticAccelerationScale,RAGE_CAR_FIELD_AUTOMATIC_ACCELERATION_SCALE,FIELD_SHORT,1),
    FIELD("top_gear",specification.topGear,RAGE_CAR_FIELD_TOP_GEAR,FIELD_SHORT,1),
    FIELD("redline",specification.redline,RAGE_CAR_FIELD_REDLINE,FIELD_SHORT,1),
    FIELD("steering_grip_response",specification.steeringGripResponse,RAGE_CAR_FIELD_STEERING_GRIP_RESPONSE,FIELD_SHORT,1),
    FIELD("steer_response",specification.steerResponse,RAGE_CAR_FIELD_STEER_RESPONSE,FIELD_USHORT,1),
    FIELD("reference_turn_radius",specification.referenceTurnRadius,RAGE_CAR_FIELD_REFERENCE_TURN_RADIUS,FIELD_SHORT,1),
    FIELD("negcon_steering_assist_scale",specification.negconSteeringAssistScale,RAGE_CAR_FIELD_NEGCON_STEERING_ASSIST_SCALE,FIELD_SHORT,1),
    FIELD("speed_drag_divisor",specification.speedDragDivisor,RAGE_CAR_FIELD_SPEED_DRAG_DIVISOR,FIELD_SHORT,1),
    FIELD("base_steering_grip",specification.baseSteeringGrip,RAGE_CAR_FIELD_BASE_STEERING_GRIP,FIELD_SHORT,1)
};
#undef FIELD

const CarField *CarCatalogFields(size_t *count) {
    if(count)*count=sizeof(fields)/sizeof(fields[0]);
    return fields;
}

static size_t FieldWidth(const CarField *field) {
    return field->type==FIELD_TEXT?1:field->type==FIELD_SHORT||field->type==FIELD_USHORT?2:4;
}

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
    if (catalog == NULL || catalog->count == 0 || catalog->count > RAGE_CAR_CATALOG_ENTRY_COUNT)
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
    errno = 0;
    number = strtol(value, &end, 10);
    if (errno == ERANGE || *end != '\0' || number < -2147483647L - 1 || number > 2147483647L)
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
        errno = 0;
        number = strtol(cursor, &end, 10);
        if (errno == ERANGE || end == cursor || number < -2147483647L - 1 || number > 2147483647L)
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

static int SetEntryValue(RageCarCatalogEntry *entry, const char *key,
                         const char *value) {
    size_t f,n;
    for(f=0;f<sizeof(fields)/sizeof(fields[0]);f++) {
        const CarField *field=&fields[f];
        unsigned char *out=(unsigned char *)entry+field->offset;
        int32_t values[16];
        if(strcmp(key,field->name))continue;
        if(entry->fields&field->bit)return 0;
        if(field->type==FIELD_TEXT) {
            if(!CopyString((char *)out,field->count,value))return 0;
        } else {
            if(field->count>1) {
                if(!ParseIntArray(value,values,field->count))return 0;
            } else {
                int v;
                if(!(field->type==FIELD_BOOL?ParseBool(value,&v):ParseInt(value,&v)))return 0;
                values[0]=v;
            }
            for(n=0;n<field->count;n++) {
                if(field->type==FIELD_SHORT&&(values[n]<INT16_MIN||values[n]>INT16_MAX))return 0;
                if(field->type==FIELD_USHORT&&(values[n]<0||values[n]>UINT16_MAX))return 0;
            }
            for(n=0;n<field->count;n++) {
                if(field->type==FIELD_SHORT){int16_t v=(int16_t)values[n];memcpy(out+n*2,&v,2);}
                else if(field->type==FIELD_USHORT){uint16_t v=(uint16_t)values[n];memcpy(out+n*2,&v,2);}
                else memcpy(out+n*4,&values[n],4);
            }
        }
        entry->fields|=field->bit;return 1;
    }
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
        if (!SetEntryValue(entry, key, value)) {
            Fail(error, errorSize, "invalid value for %s", key);
            free(copy);
            return 0;
        }
    }
    free(copy);
    return CarCatalogValidate(catalog, error, errorSize);
}

int CarCatalogVariant(int modelIndex, int grade) {
    int first,end;
    if(modelIndex<0||modelIndex>=GAME_CAR_COUNT||grade<0)return -1;
    first=kFirstVariant[modelIndex];
    end=modelIndex+1<GAME_CAR_COUNT?kFirstVariant[modelIndex+1]:CAR_MODEL_VARIANT_COUNT;
    return grade<end-first?first+grade:-1;
}

int CarCatalogReadFile(const char *path, RageCarCatalog *catalog, char *error, size_t errorSize) {
    FILE *file; long size; char *text; RageCarCatalog next;
    if (catalog == NULL || path == NULL || (file = fopen(path, "rb")) == NULL) return Fail(error,errorSize,"cannot open car catalog");
    if (fseek(file, 0, SEEK_END) != 0 || (size = ftell(file)) < 0 || fseek(file, 0, SEEK_SET) != 0 || size > 1024 * 1024) { fclose(file); return Fail(error,errorSize,"invalid car catalog size"); }
    text = malloc((size_t)size + 1); if (text == NULL) { fclose(file); return Fail(error,errorSize,"out of memory reading catalog"); }
    {
        size_t read = fread(text, 1, (size_t)size, file);
        int closed = fclose(file);
        if (read != (size_t)size || closed != 0) { free(text); return Fail(error,errorSize,"cannot read car catalog"); }
    }
    if(memchr(text,0,(size_t)size)){free(text);return Fail(error,errorSize,"embedded null in car catalog");}
    text[size] = '\0';
    if (!CarCatalogParse(text, &next, error, errorSize)) { free(text); return 0; }
    free(text);
    *catalog = next; return 1;
}

int CarCatalogMerge(RageCarCatalog *base,const RageCarCatalog *overlay,
                    char *error,size_t errorSize) {
    RageCarCatalog next;
    size_t i,j,f;
    if(!CarCatalogValidate(base,error,errorSize)||!CarCatalogValidate(overlay,error,errorSize))return 0;
    next=*base;
    for(i=0;i<overlay->count;i++) {
        const RageCarCatalogEntry *from=&overlay->entries[i];
        RageCarCatalogEntry *to;
        for(j=0;j<next.count;j++)if(next.entries[j].modelIndex==from->modelIndex&&next.entries[j].grade==from->grade)break;
        if(j==next.count) {
            if(next.count==RAGE_CAR_CATALOG_ENTRY_COUNT)return Fail(error,errorSize,"too many merged cars");
            memset(&next.entries[next.count++],0,sizeof(next.entries[0]));
        }
        to=&next.entries[j];
        for(f=0;f<sizeof(fields)/sizeof(fields[0]);f++)if(from->fields&fields[f].bit) {
            memcpy((unsigned char *)to+fields[f].offset,(const unsigned char *)from+fields[f].offset,FieldWidth(&fields[f])*fields[f].count);
        }
        to->fields|=from->fields;
    }
    if(!CarCatalogValidate(&next,error,errorSize))return 0;
    *base=next;return 1;
}

int CarCatalogWriteFile(const char *path,const RageCarCatalog *catalog,
                        char *error,size_t errorSize) {
    FILE *file;
    size_t i,f,n;
    int ok=1;
    if(!path||!CarCatalogValidate(catalog,error,errorSize))return 0;
    /* Reject text the deliberately small retail-profile grammar cannot
     * round-trip before touching the output, including hidden comment markers. */
    for(i=0;i<catalog->count;i++)for(f=0;f<sizeof(fields)/sizeof(fields[0]);f++) {
        const CarField *field=&fields[f];
        const char *text=(const char *)&catalog->entries[i]+field->offset;
        if(field->type!=FIELD_TEXT||!(catalog->entries[i].fields&field->bit))continue;
        if(!memchr(text,0,field->count))return Fail(error,errorSize,"unterminated profile text");
        for(;*text;text++)if((unsigned char)*text<32||*text=='"'||*text=='\\'||*text=='#')return Fail(error,errorSize,"unsupported profile text character");
    }
    file=fopen(path,"wbx");
    if(!file)return Fail(error,errorSize,"cannot create car catalog");
    for(i=0;i<catalog->count&&ok;i++) {
        const RageCarCatalogEntry *entry=&catalog->entries[i];
        if(fputs("[[cars]]\n",file)==EOF)ok=0;
        for(f=0;f<sizeof(fields)/sizeof(fields[0])&&ok;f++) {
            const CarField *field=&fields[f];const unsigned char *p=(const unsigned char *)entry+field->offset;
            if(!(entry->fields&field->bit))continue;
            if(fprintf(file,"%s = ",field->name)<0)ok=0;
            if(field->type==FIELD_TEXT){if(fprintf(file,"\"%s\"",(const char *)p)<0)ok=0;}
            else {
                if(field->count>1&&fputc('[',file)==EOF)ok=0;
                for(n=0;n<field->count;n++) {
                    int value;
                    if(field->type==FIELD_SHORT){int16_t v;memcpy(&v,p+n*2,2);value=v;}
                    else if(field->type==FIELD_USHORT){uint16_t v;memcpy(&v,p+n*2,2);value=v;}
                    else memcpy(&value,p+n*4,4);
                    if(n&&fputs(", ",file)==EOF)ok=0;
                    if(field->type==FIELD_BOOL){if(fputs(value?"true":"false",file)==EOF)ok=0;}
                    else if(fprintf(file,"%d",value)<0)ok=0;
                }
                if(field->count>1&&fputc(']',file)==EOF)ok=0;
            }
            if(fputc('\n',file)==EOF)ok=0;
        }
        if(fputc('\n',file)==EOF)ok=0;
    }
    if(fclose(file))ok=0;
    if(!ok){remove(path);return Fail(error,errorSize,"cannot write car catalog");}
    return 1;
}
