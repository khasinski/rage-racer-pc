#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "content_options.h"

char *g_NativeCarNames[13];
typedef struct PrologueLine {
    short x;
    short y;
    unsigned char *text;
} PrologueLine;
PrologueLine g_PrologueLines[17];
int g_PrologueLineCount = 14;
static int failures;

#define EXPECT_NAME(expected, actual) do {                                    \
    if (strcmp((expected), (actual)) != 0) {                                  \
        fprintf(stderr, "%s:%d: expected %s, got %s\n", __FILE__, __LINE__,  \
                (expected), (actual));                                        \
        failures++;                                                           \
    }                                                                         \
} while (0)

const char *RuntimeConfigGet(const char *key) {
    return strcmp(key, "content.car_names") == 0 ||
        strcmp(key, "content.prologue") == 0 ? "japanese" : NULL;
}

int main(void) {
    static const char *international[13] = {
        "ERRISO", "ABEILLE", "PEGASE", "ESPERANZA", "ACCERON", "BAYONET",
        "HIJACK", "FATALITA", "ISTANTE", "GHEPARDO", "VAINQURE", "BULSHADE",
        "SQUALDON",
    };
    static const char *japanese[13] = {
        "ALOUETTE", "ABEILLE", "PEGASE", "ESPERANZA", "INSTINCT", "BAYONET",
        "HIJACK", "FATALITA", "ISTANTE", "GHEPARDO", "VICTOIRE", "TEMPEST",
        "DRAGONE",
    };
    int index;

    EXPECT_NAME("ERRISO", ContentCarNameForStyle(0, "ERRISO", NULL));
    EXPECT_NAME("ERRISO", ContentCarNameForStyle(
        0, "ERRISO", "international"));
    EXPECT_NAME("ERRISO", ContentCarNameForStyle(
        0, "ERRISO", "unknown"));
    for (index = 0; index < 13; index++)
        g_NativeCarNames[index] = (char *)international[index];
    ContentOptionsApply();
    for (index = 0; index < 13; index++)
        EXPECT_NAME(japanese[index], g_NativeCarNames[index]);
    if (g_PrologueLineCount != 17) {
        fprintf(stderr, "expected 17 Japanese prologue lines, got %d\n",
                g_PrologueLineCount);
        failures++;
    }
    EXPECT_NAME("RAGE RACER....", (char *)g_PrologueLines[0].text);
    EXPECT_NAME("THE #1 RAGE RACER.", (char *)g_PrologueLines[16].text);
    if (g_PrologueLines[0].x != 104 || g_PrologueLines[16].y != 314) {
        fprintf(stderr, "Japanese prologue alignment was not applied\n");
        failures++;
    }
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
