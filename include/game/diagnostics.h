#ifndef GAME_DIAGNOSTICS_H
#define GAME_DIAGNOSTICS_H

/* Diagnostics boundary for recovered game code. Keys are stable engine
 * concepts; the host port owns INI and legacy environment-variable mapping. */
int DiagnosticsEnabled(const char *key);
const char *DiagnosticsValue(const char *key);
/* Parse a complete base-0 integer, returning fallback for absent, malformed,
 * overflowing, or out-of-int-range settings. */
int DiagnosticsIntValue(const char *key, int fallback);

/*
 * Write one trace line: the topic, then `key=value` fields, then a newline.
 *
 * Traces used to be printf in the recovered game code and fprintf(stderr) in
 * the host. Those are two different destinations once the diagnostic log takes
 * stderr over, so answering one question meant collecting output from two
 * places and hoping the orderings lined up. This puts every line in the log,
 * with the topic separated from the payload so a grep for a topic cannot also
 * match a value that happens to contain it.
 */
void Trace(const char *topic, const char *format, ...);

#endif
