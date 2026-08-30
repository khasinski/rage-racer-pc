/*
 * The two content settings swap the international release's car names and
 * prologue for the Japanese release's. Both settings have to be safe to get
 * wrong: an unrecognised or missing value has to leave the shipped content
 * exactly as it was, because the alternative is a half-applied swap.
 *
 * Only the japanese setting used to be exercised here, so every path that
 * declines to swap was unlocked, as was the placement of fifteen of the
 * seventeen prologue lines.
 */

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

static void Fail(const char *what, const char *expected, const char *got) {
    printf("FAIL %s: expected %s, got %s\n", what, expected, got);
    failures++;
}

static void ExpectName(const char *what, const char *expected,
                       const char *got) {
    if (strcmp(expected, got) != 0) Fail(what, expected, got);
}

static void ExpectInt(const char *what, int expected, int got) {
    if (expected == got) return;
    printf("FAIL %s: expected %d, got %d\n", what, expected, got);
    failures++;
}

/* What the settings file is taken to say for one run. */
static const char *s_carNames;
static const char *s_prologue;

const char *RuntimeConfigGet(const char *key) {
    if (strcmp(key, "content.car_names") == 0) return s_carNames;
    if (strcmp(key, "content.prologue") == 0) return s_prologue;
    return NULL;
}

static const char *kInternational[13] = {
    "ERRISO", "ABEILLE", "PEGASE", "ESPERANZA", "ACCERON", "BAYONET",
    "HIJACK", "FATALITA", "ISTANTE", "GHEPARDO", "VAINQURE", "BULSHADE",
    "SQUALDON",
};
static const char *kJapanese[13] = {
    "ALOUETTE", "ABEILLE", "PEGASE", "ESPERANZA", "INSTINCT", "BAYONET",
    "HIJACK", "FATALITA", "ISTANTE", "GHEPARDO", "VICTOIRE", "TEMPEST",
    "DRAGONE",
};

/* The Japanese prologue, and where each line has to land: the y comes from the
 * release's own table and the x centres the line in the 320-wide screen at
 * eight pixels a character. */
static const struct {
    const char *text;
    short y;
} kJapanesePrologue[17] = {
    {"RAGE RACER....", 6},
    {"THE DEEP PRIMITIVE ROARING", 32},
    {"EXHAUST NOTES TITILLATE THE", 49},
    {"BASE INSTINCTS OF THOSE WHO", 66},
    {"BECOME KNOWN AS RAGE RACERS.", 83},
    {"NO-ONE KNOWS HOW THE RACE", 109},
    {"STARTED OR HOW THE CONTESTANTS", 126},
    {"BECOME KNOWN AS RAGE RACERS.", 143},
    {"CONTESTANTS DANGEROUSLY LIVING", 169},
    {"ON THE EDGE, THOSE WHO LIVE FOR", 186},
    {"THE MOMENT AND LOVE THE HEADY", 203},
    {"PERFUME OF NITRO,SMOKED RUBBER", 220},
    {"AND HOT ASPHALT. MEETING", 237},
    {"TOGETHER FOR ONE PURPOSE TO BE", 254},
    {"THE BEST THERE IS....", 271},
    {"THE ULTIMATE....", 288},
    {"THE #1 RAGE RACER.", 314},
};

/* Shipped content, so a setting that declines to swap has something to leave
 * alone that is visibly not the Japanese version. */
static void LoadShippedContent(void) {
    int i;
    for (i = 0; i < 13; i++) g_NativeCarNames[i] = (char *)kInternational[i];
    for (i = 0; i < 17; i++) {
        g_PrologueLines[i].x = (short)(1000 + i);
        g_PrologueLines[i].y = (short)(2000 + i);
        g_PrologueLines[i].text = (unsigned char *)"SHIPPED";
    }
    g_PrologueLineCount = 14;
}

static void ExpectContentUntouched(const char *what) {
    char label[128];
    int i;
    for (i = 0; i < 13; i++) {
        snprintf(label, sizeof(label), "%s leaves car %d alone", what, i);
        ExpectName(label, kInternational[i], g_NativeCarNames[i]);
    }
    snprintf(label, sizeof(label), "%s leaves the prologue text alone", what);
    ExpectName(label, "SHIPPED", (char *)g_PrologueLines[0].text);
    snprintf(label, sizeof(label), "%s leaves the prologue length alone", what);
    ExpectInt(label, 14, g_PrologueLineCount);
    snprintf(label, sizeof(label), "%s leaves the prologue placement alone",
             what);
    ExpectInt(label, 1000, g_PrologueLines[0].x);
}

/* Every value that must not swap anything, whether it is absent, blank, the
 * shipped release or something nobody recognises. */
static void DecliningSettingsTests(void) {
    static const char *const leaveAlone[] = {NULL, "", "international",
                                             "JAPANESE", "japan", "nihongo"};
    size_t i;

    for (i = 0; i < sizeof(leaveAlone) / sizeof(leaveAlone[0]); i++) {
        LoadShippedContent();
        s_carNames = leaveAlone[i];
        s_prologue = leaveAlone[i];
        ContentOptionsApply();
        ExpectContentUntouched(leaveAlone[i] == NULL ? "an unset setting"
                                                     : leaveAlone[i]);
    }
}

/* The two settings are independent: asking for one release's names must not
 * bring the other release's prologue with it. */
static void SettingsAreIndependentTests(void) {
    int i;

    LoadShippedContent();
    s_carNames = "japanese";
    s_prologue = "international";
    ContentOptionsApply();
    ExpectName("names alone swaps the name", kJapanese[0],
               g_NativeCarNames[0]);
    ExpectName("names alone leaves the prologue", "SHIPPED",
               (char *)g_PrologueLines[0].text);
    ExpectInt("names alone leaves the prologue length", 14,
              g_PrologueLineCount);

    LoadShippedContent();
    s_carNames = "international";
    s_prologue = "japanese";
    ContentOptionsApply();
    ExpectName("prologue alone leaves the names", kInternational[0],
               g_NativeCarNames[0]);
    ExpectName("prologue alone swaps the prologue",
               kJapanesePrologue[0].text, (char *)g_PrologueLines[0].text);
    for (i = 0; i < 13; i++)
        ExpectName("prologue alone leaves every name", kInternational[i],
                   g_NativeCarNames[i]);
}

/* Only five of the thirteen cars were renamed for the international release;
 * the rest have to come through the swap unchanged. */
static void CarNameTests(void) {
    int i;

    for (i = 0; i < 13; i++) {
        ExpectName("no style keeps the shipped name", kInternational[i],
                   ContentCarNameForStyle(i, kInternational[i], NULL));
        ExpectName("the shipped style keeps the shipped name",
                   kInternational[i],
                   ContentCarNameForStyle(i, kInternational[i],
                                          "international"));
        ExpectName("an unknown style keeps the shipped name",
                   kInternational[i],
                   ContentCarNameForStyle(i, kInternational[i], "unknown"));
        ExpectName("the Japanese style renames what was renamed",
                   kJapanese[i],
                   ContentCarNameForStyle(i, kInternational[i], "japanese"));
    }

    /* A model the table says nothing about keeps whatever it was given. */
    ExpectName("a model outside the table keeps its name", "SQUALDON",
               ContentCarNameForStyle(99, "SQUALDON", "japanese"));

    /* Applying the swap twice must not rename an already-renamed car. */
    LoadShippedContent();
    s_carNames = "japanese";
    s_prologue = NULL;
    ContentOptionsApply();
    ContentOptionsApply();
    for (i = 0; i < 13; i++)
        ExpectName("applying twice is applying once", kJapanese[i],
                   g_NativeCarNames[i]);
}

/* Every line of the Japanese prologue, its text, the row it sits on and the
 * centring that follows from its length. */
static void PrologueTests(void) {
    char label[128];
    int i;

    LoadShippedContent();
    s_carNames = NULL;
    s_prologue = "japanese";
    ContentOptionsApply();

    ExpectInt("the Japanese prologue is seventeen lines", 17,
              g_PrologueLineCount);
    for (i = 0; i < 17; i++) {
        int length = (int)strlen(kJapanesePrologue[i].text);
        snprintf(label, sizeof(label), "prologue line %d text", i);
        ExpectName(label, kJapanesePrologue[i].text,
                   (char *)g_PrologueLines[i].text);
        snprintf(label, sizeof(label), "prologue line %d row", i);
        ExpectInt(label, kJapanesePrologue[i].y, g_PrologueLines[i].y);
        snprintf(label, sizeof(label), "prologue line %d centring", i);
        ExpectInt(label, (320 - length * 8) / 2, g_PrologueLines[i].x);
    }
}

/*
 * A setting nobody recognises is worth a word to the player, but an unset or
 * blank one is the normal case and has to pass in silence. The two only
 * differ on stderr, so this is where the difference is visible at all.
 */
#define STDERR_CAPTURE "content_options_stderr.tmp"

static long CapturedLength(void) {
    long length;
    FILE *file;
    fflush(stderr);
    file = fopen(STDERR_CAPTURE, "rb");
    if (file == NULL) return -1;
    fseek(file, 0, SEEK_END);
    length = ftell(file);
    fclose(file);
    return length;
}

static void WarningTests(void) {
    static const struct {
        const char *setting;
        int warns;
    } cases[] = {
        {NULL, 0}, {"", 0}, {"international", 0}, {"japanese", 1},
        {"nihongo", 1}, {"JAPANESE", 1},
    };
    size_t i;

    if (freopen(STDERR_CAPTURE, "w", stderr) == NULL) {
        printf("FAIL could not capture stderr\n");
        failures++;
        return;
    }
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        long before, after;
        LoadShippedContent();
        if (freopen(STDERR_CAPTURE, "w", stderr) == NULL) {
            printf("FAIL could not reopen the capture\n");
            failures++;
            return;
        }
        before = CapturedLength();
        s_carNames = cases[i].setting;
        s_prologue = NULL;
        ContentOptionsApply();
        after = CapturedLength();
        ExpectInt(cases[i].setting == NULL
                      ? "an unset name setting says nothing"
                      : cases[i].setting,
                  cases[i].warns, after > before);
    }
    /* The prologue setting has its own guard and has to be as quiet. */
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        long before, after;
        LoadShippedContent();
        if (freopen(STDERR_CAPTURE, "w", stderr) == NULL) {
            printf("FAIL could not reopen the capture\n");
            failures++;
            return;
        }
        before = CapturedLength();
        s_carNames = NULL;
        s_prologue = cases[i].setting;
        ContentOptionsApply();
        after = CapturedLength();
        ExpectInt(cases[i].setting == NULL
                      ? "an unset prologue setting says nothing"
                      : cases[i].setting,
                  cases[i].warns, after > before);
    }
    remove(STDERR_CAPTURE);
}

int main(void) {
    DecliningSettingsTests();
    SettingsAreIndependentTests();
    CarNameTests();
    PrologueTests();
    WarningTests();

    if (failures != 0) {
        printf("%d content option assertion(s) failed\n", failures);
        return EXIT_FAILURE;
    }
    printf("the content settings swap both releases and decline safely\n");
    return EXIT_SUCCESS;
}
