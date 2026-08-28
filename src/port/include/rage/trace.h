#ifndef RAGE_TRACE_H
#define RAGE_TRACE_H

struct PlayerCarRuntime;

/* Config-gated diagnostics kept outside the renderer-neutral game sources. */
void TraceCarMotion(const char *phase, struct PlayerCarRuntime *car);
void TraceCarStates(void);

#endif
