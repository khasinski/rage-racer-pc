#ifndef RAGE_TRACE_H
#define RAGE_TRACE_H

struct PlayerCarRuntime;

/* Config-gated diagnostics kept outside the renderer-neutral game sources. */
void RageTraceCarMotion(const char *phase, struct PlayerCarRuntime *car);
void RageTraceCarStates(void);

#endif
