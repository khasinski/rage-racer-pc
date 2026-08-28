#ifndef RAGE_RUNTIME_CONFIG_H
#define RAGE_RUNTIME_CONFIG_H

/* Process-wide configuration assembled once at startup. Files use INI
 * sections and dotted keys (for example [race] mode = grand-prix becomes
 * race.mode). --set key=value overrides files. */
int RageRuntimeConfigInit(int argc, char **argv);
const char *RageRuntimeConfigGet(const char *key);
const char *RageRuntimeConfigGetLegacy(const char *key, const char *legacyEnv);
const char *RageRuntimeConfigGetOverride(const char *key, const char *overrideEnv);
int RageRuntimeConfigEnabled(const char *key, const char *legacyEnv);

#endif
