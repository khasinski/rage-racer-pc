#ifndef RAGE_RUNTIME_CONFIG_H
#define RAGE_RUNTIME_CONFIG_H

#include "runtime_parse.h"

/* Process-wide configuration assembled once at startup. Files use INI
 * sections and dotted keys (for example [race] mode = grand-prix becomes
 * race.mode). --set key=value overrides files. */
int RuntimeConfigInit(int argc, char **argv);
/*
 * Every setting can also be given in the environment; runtime_config.c derives
 * the name from the key and keeps the historical exceptions in one table, so
 * no call site names an environment variable.
 */
const char *RuntimeConfigGet(const char *key);
int RuntimeConfigEnabled(const char *key);

/* Environment wins over the file. For the few settings an outer harness must
 * be able to force whatever the shipped configuration says. */
const char *RuntimeConfigGetForced(const char *key);

/* Parse a complete decimal/hex integer and constrain it to the caller's
 * domain. Missing and malformed settings return fallback. */
int RuntimeConfigInt(const char *key, int fallback, int minimum, int maximum);

#endif
