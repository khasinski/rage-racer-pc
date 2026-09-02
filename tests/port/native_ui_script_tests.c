#include <stddef.h>
#include <stdio.h>

#include "game/menu.h"

enum { UI_SCRIPT_DATA_SIZE = 0x321c };

extern const unsigned char g_UiScriptData[UI_SCRIPT_DATA_SIZE];
int InitNativeGameData(void);

static int IsUiDataPointer(const void *pointer) {
    const unsigned char *bytes = pointer;
    return bytes >= g_UiScriptData &&
           bytes < g_UiScriptData + UI_SCRIPT_DATA_SIZE;
}

static unsigned long UiDataDigest(void) {
    unsigned long digest = 2166136261UL;
    size_t i;

    for (i = 0; i < UI_SCRIPT_DATA_SIZE; i++) {
        digest ^= g_UiScriptData[i];
        digest = (digest * 16777619UL) & 0xFFFFFFFFUL;
    }
    return digest;
}

static int CheckScript(const char *name, const TimedDrawCommand *script,
                       size_t count) {
    size_t i;

    for (i = 0; i < count; i++) {
        if (script[i].time < 0) continue;
        if (!IsUiDataPointer(script[i].shape.pointer) ||
            !IsUiDataPointer(script[i].motion.pointer)) {
            printf("FAIL %s command %lu was not relocated into UI data\n",
                   name, (unsigned long)i);
            return 0;
        }
    }
    return 1;
}

int main(void) {
    unsigned long digest = UiDataDigest();

    if (digest != 3096134374UL) {
        printf("FAIL UI data digest is %lu\n", digest);
        return 1;
    }
    if (!InitNativeGameData()) {
        puts("FAIL native UI data did not initialize");
        return 1;
    }

    if (!CheckScript("course select", g_CourseSelectGpScript, 10) ||
        !CheckScript("ranking", g_RankingPanelScript, 5) ||
        !CheckScript("team name", g_TeamNameScreenScript, 61) ||
        !CheckScript("engineer shop", g_EngineerShopScreenScript, 68) ||
        !CheckScript("menu hints", g_MenuHintBarScript, 61)) {
        return 1;
    }

    puts("native UI scripts reference relocated UI data");
    return 0;
}
