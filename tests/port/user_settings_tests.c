#include "user_settings.h"
#include "runtime_config.h"
#include "platform_paths.h"
#include <SDL3/SDL.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s (%s)\n", __LINE__, #x, SDL_GetError()); return 1; } } while (0)
static char bundled[4096];
static char profile[4096];
static int SetTestEnvironment(const char *name, const char *value) {
#ifdef _WIN32
    /* SDL updates the Win32 environment; RuntimeConfig uses the CRT's copy.
     * Keep both consistent when a test changes environment after startup. */
    if (_putenv_s(name, value != NULL ? value : "") != 0) return -1;
#endif
    return value != NULL ? SDL_setenv_unsafe(name, value, 1) : SDL_unsetenv_unsafe(name);
}
int PlatformFindConfigFile(const char *argv0, const char *name, char *out, size_t size) {
    (void)argv0; (void)name;
    if (!bundled[0]) return 0;
    SDL_strlcpy(out, bundled, size);
    return 1;
}
int PlatformUserConfigPath(const char *name, char *out, size_t size) {
    (void)name;
    if (!profile[0]) return 0;
    SDL_strlcpy(out, profile, size);
    return 1; /* Always points inside the isolated test directory. */
}

static int ReadPreferences(const char *path) {
    char setting[4200];
    snprintf(setting, sizeof(setting), "settings.path=%s", path);
    char *args[] = {"settings-test", "--set", setting};
    CHECK(RuntimeConfigInit(3, args));
    RagePortConfig config;
    PortConfigDefaults(&config);
    PortConfigApplyRuntime(&config);
    CHECK(fabsf(config.modernBrightness - 1.25f) < 0.001f);
    CHECK(config.modernFps == 144);
    CHECK(RuntimeConfigEnabled("video.fullscreen"));
    CHECK(!strcmp(RuntimeConfigGet("hud.anchor"), "center"));
    CHECK(!strcmp(RuntimeConfigGet("camera.chase_distance"), "2"));
    CHECK(!RuntimeConfigGet("disc.image") && !RuntimeConfigGet("tools.hd_pack_create"));
    return 0;
}

int main(int argc, char **argv) {
    if (argc == 3 && !strcmp(argv[1], "read")) return ReadPreferences(argv[2]);
    CHECK(argc == 2);
    char root[4000], path[4096], setting[4200], explicitConfig[4096], bad[4200];
    int rootLength = snprintf(root, sizeof(root), "%s/%llu", argv[1], (unsigned long long)SDL_GetTicksNS());
    CHECK(rootLength > 0 && (size_t)rootLength < sizeof(root));
    CHECK(SDL_CreateDirectory(root));
    snprintf(path, sizeof(path), "%s/prefs/settings.ini", root);
    RagePortConfig config = PortConfigDefaultValues();
    PortSessionOptions session = {0};
    config.modernBrightness = 1.25f;
    config.modernFps = 144;
    session.fullscreen = 1;
    session.chaseDistance = 2;
    session.chaseHeight = 1;
    session.tickMultiplier = 1;
    session.steeringLinearity = 0.5f;
    CHECK(UserSettingsSave(path, &config, &session));
    config.modernBrightness = 1.2f;
    CHECK(UserSettingsSave(path, &config, &session));
    config.modernBrightness = 1.25f;
    CHECK(UserSettingsSave(path, &config, &session));
    CHECK(ReadPreferences(path) == 0);
    const char *childArgs[] = {argv[0], "read", path, NULL};
    SDL_Process *child = SDL_CreateProcess(childArgs, false);
    CHECK(child);
    int code = -1;
    CHECK(SDL_WaitProcess(child, true, &code));
    SDL_DestroyProcess(child);
    CHECK(code == 0); /* An independent process restores the same choices. */

    snprintf(bundled, sizeof(bundled), "%s/bundled.ini", root);
    const char *defaults = "[video]\nbrightness=0.75\nfps=60\n";
    CHECK(SDL_SaveFile(bundled, defaults, strlen(defaults)));
    CHECK(ReadPreferences(path) == 0); /* Saved choices beat bundled defaults. */
    snprintf(explicitConfig, sizeof(explicitConfig), "%s/explicit.ini", root);
    const char *explicitValues = "[video]\nbrightness=0.9\nfps=120\n";
    CHECK(SDL_SaveFile(explicitConfig, explicitValues, strlen(explicitValues)));
    snprintf(setting, sizeof(setting), "settings.path=%s", path);
    char *args[] = {"settings-test", "--set", setting, "--config", explicitConfig,
                    "--set", "video.brightness=1.4"};
    CHECK(RuntimeConfigInit(7, args));
    PortConfigDefaults(&config);
    PortConfigApplyRuntime(&config);
    CHECK(fabsf(config.modernBrightness - 1.4f) < 0.001f && config.modernFps == 120);

    size_t previousSize, afterSize;
    void *previous = SDL_LoadFile(path, &previousSize);
    CHECK(previous);
    snprintf(bad, sizeof(bad), "%s/child.ini", path);
    CHECK(!UserSettingsSave(bad, &config, &session)); /* Parent is a file. */
    void *after = SDL_LoadFile(path, &afterSize);
    CHECK(after && previousSize == afterSize && !memcmp(previous, after, afterSize));
    SDL_free(previous); SDL_free(after);

    FILE *file = fopen(path, "a");
    CHECK(file);
    fputs("[disc]\nimage=unexpected.bin\n[tools]\nhd_pack_create=unexpected\n", file);
    CHECK(fclose(file) == 0);
    CHECK(ReadPreferences(path) == 0); /* Profile cannot inject tool/disc options. */
    file = fopen(path, "a");
    CHECK(file);
    fputs("[video]\nbrightness=nan\nrenderer=invalid\n", file);
    CHECK(fclose(file) == 0);
    char *malformedArgs[] = {"settings-test", "--set", setting};
    CHECK(RuntimeConfigInit(3, malformedArgs));
    PortConfigDefaults(&config);
    PortConfigApplyRuntime(&config);
    CHECK(config.modernBrightness == 1 && config.renderer == RAGE_RENDERER_MODERN);
    char *isolated[] = {"settings-test", "--set", "settings.path="};
    CHECK(RuntimeConfigInit(3, isolated));
    CHECK(!RuntimeConfigSettingsPath());
    config.modernBrightness = 1.25f;
    config.modernFps = 144;
    CHECK(UserSettingsSave(path, &config, &session));
    SDL_strlcpy(profile, path, sizeof(profile));
    CHECK(SetTestEnvironment("RAGE_PORT_SETTINGS_PATH", NULL) == 0);
    CHECK(SetTestEnvironment("PSYZ_HEADLESS", "0") == 0);
    RuntimeConfigEnableUserSettings();
    char *normal[] = {"settings-test"};
    CHECK(RuntimeConfigInit(1, normal));
    PortConfigDefaults(&config);
    PortConfigApplyRuntime(&config);
    CHECK(config.modernFps == 144 && fabsf(config.modernBrightness - 1.25f) < 0.001f);
    RuntimeConfigIsolateUserSettings();
    CHECK(RuntimeConfigInit(1, normal));
    PortConfigDefaults(&config);
    PortConfigApplyRuntime(&config);
    CHECK(config.modernFps == 60 && config.modernBrightness == 0.75f);
    CHECK(!RuntimeConfigSettingsPath());
    CHECK(SetTestEnvironment("PSYZ_HEADLESS", "1") == 0);
    puts("Settings: restart, precedence, failure preservation and malformed data passed");
    return 0;
}
