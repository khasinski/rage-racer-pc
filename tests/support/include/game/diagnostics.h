#ifndef GAME_DIAGNOSTICS_H
#define GAME_DIAGNOSTICS_H
int DiagnosticsEnabled(const char *key);
const char *DiagnosticsValue(const char *key);
/* Mirrors the production diagnostics boundary used by recovered game code. */
int DiagnosticsIntValue(const char *key, int fallback);
#endif
