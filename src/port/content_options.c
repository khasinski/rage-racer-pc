#include "content_options.h"

#include <stdio.h>
#include <string.h>

#include "runtime_config.h"

extern char *g_NativeCarNames[13];

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

const char *RageContentCarNameForStyle(int modelIndex,
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

void RageContentOptionsApply(void) {
    const char *style = RageRuntimeConfigGet("content.car_names");
    int modelIndex;
    if (style == NULL || style[0] == '\0' ||
        strcmp(style, "international") == 0) return;
    if (strcmp(style, "japanese") != 0) {
        fprintf(stderr,
                "rage-port: content.car_names must be international or japanese; using international\n");
        return;
    }
    for (modelIndex = 0; modelIndex < 13; modelIndex++) {
        g_NativeCarNames[modelIndex] = (char *)RageContentCarNameForStyle(
            modelIndex, g_NativeCarNames[modelIndex], style);
    }
    fprintf(stderr, "rage-port: using Japanese-release car names\n");
}
