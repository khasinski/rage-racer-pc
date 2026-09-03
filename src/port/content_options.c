
#include <stdio.h>
#include <string.h>

#include "game/car.h"
#include "game/race.h"
#include "runtime_config.h"

typedef struct RageRegionalCarName {
    int modelIndex;
    const char *japaneseName;
} RageRegionalCarName;

static const RageRegionalCarName kRegionalCarNames[] = {
    {0, "ALOUETTE"},
    {4, "INSTINCT"},
    {10, "VICTOIRE"},
    {11, "TEMPEST"},
    {12, "DRAGONE"},
};

static const char *const kJapanesePrologueLines[PROLOGUE_LINE_CAPACITY] = {
    "RAGE RACER....",
    "THE DEEP PRIMITIVE ROARING",
    "EXHAUST NOTES TITILLATE THE",
    "BASE INSTINCTS OF THOSE WHO",
    "BECOME KNOWN AS RAGE RACERS.",
    "NO-ONE KNOWS HOW THE RACE",
    "STARTED OR HOW THE CONTESTANTS",
    "BECOME KNOWN AS RAGE RACERS.",
    "CONTESTANTS DANGEROUSLY LIVING",
    "ON THE EDGE, THOSE WHO LIVE FOR",
    "THE MOMENT AND LOVE THE HEADY",
    "PERFUME OF NITRO,SMOKED RUBBER",
    "AND HOT ASPHALT. MEETING",
    "TOGETHER FOR ONE PURPOSE TO BE",
    "THE BEST THERE IS....",
    "THE ULTIMATE....",
    "THE #1 RAGE RACER.",
};

static const s16 kJapanesePrologueY[PROLOGUE_LINE_CAPACITY] = {
    6, 32, 49, 66, 83, 109, 126, 143, 169,
    186, 203, 220, 237, 254, 271, 288, 314,
};

const char *ContentCarNameForStyle(int modelIndex,
                                       const char *internationalName,
                                       const char *style) {
    size_t index;
    if (style == NULL || strcmp(style, "japanese") != 0)
        return internationalName;
    for (index = 0; index < sizeof(kRegionalCarNames) /
             sizeof(kRegionalCarNames[0]); index++) {
        if (kRegionalCarNames[index].modelIndex == modelIndex)
            return kRegionalCarNames[index].japaneseName;
    }
    return internationalName;
}

void ContentOptionsApply(void) {
    const char *nameStyle = RuntimeConfigGet("content.car_names");
    const char *prologueStyle = RuntimeConfigGet("content.prologue");
    int modelIndex;
    int lineIndex;
    if (nameStyle != NULL && nameStyle[0] != '\0' &&
        strcmp(nameStyle, "international") != 0 &&
        strcmp(nameStyle, "japanese") != 0) {
        fprintf(stderr,
                "rage-port: content.car_names must be international or japanese; using international\n");
    } else if (nameStyle != NULL && strcmp(nameStyle, "japanese") == 0) {
        for (modelIndex = 0; modelIndex < GAME_CAR_COUNT; modelIndex++) {
            g_NativeCarNames[modelIndex] = ContentCarNameForStyle(
                modelIndex, g_NativeCarNames[modelIndex], nameStyle);
        }
        fprintf(stderr, "rage-port: using Japanese-release car names\n");
    }

    if (prologueStyle == NULL || prologueStyle[0] == '\0' ||
        strcmp(prologueStyle, "international") == 0) return;
    if (strcmp(prologueStyle, "japanese") != 0) {
        fprintf(stderr,
                "rage-port: content.prologue must be international or japanese; using international\n");
        return;
    }
    for (lineIndex = 0; lineIndex < PROLOGUE_LINE_CAPACITY; lineIndex++) {
        size_t length = strlen(kJapanesePrologueLines[lineIndex]);
        g_PrologueLines[lineIndex].x = (short)((320 - length * 8) / 2);
        g_PrologueLines[lineIndex].y = kJapanesePrologueY[lineIndex];
        g_PrologueLines[lineIndex].text = kJapanesePrologueLines[lineIndex];
    }
    g_PrologueLineCount = PROLOGUE_LINE_CAPACITY;
    fprintf(stderr,
            "rage-port: using the extended Japanese-release prologue text\n");
}
