#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "content_options.h"

char *g_NativeCarNames[13];
static int failures;

#define EXPECT_NAME(expected, actual) do {                                    \
    if (strcmp((expected), (actual)) != 0) {                                  \
        fprintf(stderr, "%s:%d: expected %s, got %s\n", __FILE__, __LINE__,  \
                (expected), (actual));                                        \
        failures++;                                                           \
    }                                                                         \
} while (0)

const char *RageRuntimeConfigGet(const char *key) {
    return strcmp(key, "content.car_names") == 0 ? "japanese" : NULL;
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

    EXPECT_NAME("ERRISO", RageContentCarNameForStyle(0, "ERRISO", NULL));
    EXPECT_NAME("ERRISO", RageContentCarNameForStyle(
        0, "ERRISO", "international"));
    EXPECT_NAME("ERRISO", RageContentCarNameForStyle(
        0, "ERRISO", "unknown"));
    for (index = 0; index < 13; index++)
        g_NativeCarNames[index] = (char *)international[index];
    RageContentOptionsApply();
    for (index = 0; index < 13; index++)
        EXPECT_NAME(japanese[index], g_NativeCarNames[index]);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
