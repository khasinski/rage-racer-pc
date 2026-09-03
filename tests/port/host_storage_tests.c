#include "host_storage.h"

#include <stdio.h>
#include <string.h>

static int s_portableAvailable;
static int s_userAvailable;
static int s_ensureCalls;
static int s_ensureFailureCall;
static char s_ensured[3][128];
static int (*s_adjustPath)(char *, const char *, int);

int PlatformExecutableDirectory(const char *argv0, char *path,
                                size_t pathSize) {
    (void)argv0;
    return snprintf(path, pathSize, "%s", "/application") > 0;
}

int PlatformExistingPortableStateDirectory(const char *executableDirectory,
                                           char *path, size_t pathSize) {
    (void)executableDirectory;
    if (!s_portableAvailable) return 0;
    return snprintf(path, pathSize, "%s", "/portable") > 0;
}

int PlatformUserStateDirectory(char *path, size_t pathSize) {
    if (!s_userAvailable) return 0;
    return snprintf(path, pathSize, "%s", "/user") > 0;
}

int PlatformEnsureDirectory(const char *path) {
    int call = s_ensureCalls;

    if (s_ensureCalls < 3) {
        snprintf(s_ensured[s_ensureCalls], sizeof(s_ensured[0]), "%s", path);
    }
    s_ensureCalls++;
    return call != s_ensureFailureCall;
}

void Psyz_AdjustPathCB(int (*callback)(char *, const char *, int)) {
    s_adjustPath = callback;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static int TestPortableStorage(void) {
    char path[128];
#ifdef _WIN32
    const char *root = "/portable";
    const char separator = '\\';
#else
    const char *root = "/portable";
    const char separator = '/';
#endif

    s_portableAvailable = 1;
    s_userAvailable = 1;
    s_ensureCalls = 0;
    s_ensureFailureCall = -1;
    s_adjustPath = NULL;
    CHECK(HostInitStorage());
    CHECK(s_ensureCalls == 3);
    CHECK(strcmp(s_ensured[0], root) == 0);
    snprintf(path, sizeof(path), "%s%cbu00", root, separator);
    CHECK(strcmp(s_ensured[1], path) == 0);
    snprintf(path, sizeof(path), "%s%cbu10", root, separator);
    CHECK(strcmp(s_ensured[2], path) == 0);
    CHECK(s_adjustPath != NULL);
    CHECK(s_adjustPath(path, "bu10:SAVE", sizeof(path)) > 0);
    {
        char expected[128];
        snprintf(expected, sizeof(expected), "%s%cbu10%cSAVE", root,
                 separator, separator);
        CHECK(strcmp(path, expected) == 0);
    }
    CHECK(s_adjustPath(path, "bu00:*", sizeof(path)) > 0);
    CHECK(path[strlen(path) - 1] == separator);
    snprintf(path, sizeof(path), "%s", "stale");
    CHECK(s_adjustPath(path, "bu20:SAVE", sizeof(path)) < 0);
    CHECK(path[0] == '\0');
    snprintf(path, sizeof(path), "%s", "stale");
    CHECK(s_adjustPath(path, NULL, sizeof(path)) < 0);
    CHECK(path[0] == '\0');
    CHECK(s_adjustPath(path, "bu00:SAVE", 4) < 0 && path[0] == '\0');
    return 0;
}

static int TestUserStorageFallback(void) {
    s_portableAvailable = 0;
    s_userAvailable = 1;
    s_ensureCalls = 0;
    s_ensureFailureCall = -1;
    CHECK(HostInitStorage());
#ifdef _WIN32
    CHECK(strcmp(s_ensured[0], "/user") == 0);
#else
    CHECK(strcmp(s_ensured[0], "/user") == 0);
#endif
    s_userAvailable = 0;
    CHECK(!HostInitStorage());
    return 0;
}

static int TestPartialInitializationFailure(void) {
    char path[128];

    s_portableAvailable = 1;
    s_userAvailable = 1;
    s_ensureCalls = 0;
    s_ensureFailureCall = -1;
    s_adjustPath = NULL;
    CHECK(HostInitStorage());

    s_portableAvailable = 0;
    s_ensureCalls = 0;
    s_ensureFailureCall = 2;
    CHECK(!HostInitStorage());
    CHECK(s_ensureCalls == 3);
    CHECK(s_adjustPath != NULL);
    CHECK(s_adjustPath(path, "bu00:SAVE", sizeof(path)) > 0);
#ifdef _WIN32
    CHECK(strcmp(path, "/portable\\bu00\\SAVE") == 0);
#else
    CHECK(strcmp(path, "/portable/bu00/SAVE") == 0);
#endif
    return 0;
}

int main(void) {
    CHECK(TestPortableStorage() == 0);
    CHECK(TestPartialInitializationFailure() == 0);
    CHECK(TestUserStorageFallback() == 0);
    puts("host storage maps both virtual memory cards into selected state data");
    return 0;
}
