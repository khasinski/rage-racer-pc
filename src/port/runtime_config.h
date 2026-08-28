#ifndef RAGE_RUNTIME_CONFIG_H
#define RAGE_RUNTIME_CONFIG_H

/* Process-wide configuration assembled once at startup. Files use INI
 * sections and dotted keys (for example [race] mode = grand-prix becomes
 * race.mode). --set key=value overrides files. */
int RuntimeConfigInit(int argc, char **argv);
const char *RuntimeConfigGet(const char *key);
const char *RuntimeConfigGetLegacy(const char *key, const char *legacyEnv);
const char *RuntimeConfigGetOverride(const char *key, const char *overrideEnv);
int RuntimeConfigEnabled(const char *key, const char *legacyEnv);

#endif
