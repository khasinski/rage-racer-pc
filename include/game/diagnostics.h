#ifndef GAME_DIAGNOSTICS_H
#define GAME_DIAGNOSTICS_H

/* Diagnostics boundary for recovered game code. Keys are stable engine
 * concepts; the host port owns INI and legacy environment-variable mapping. */
int DiagnosticsEnabled(const char *key);
const char *DiagnosticsValue(const char *key);

#endif
