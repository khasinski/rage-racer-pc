#ifndef RAGE_MEMORY_BUDGET_H
#define RAGE_MEMORY_BUDGET_H

#include "port_config.h"

typedef enum RageMemoryTier {
    RAGE_MEMORY_NORMAL, RAGE_MEMORY_LOW, RAGE_MEMORY_MINIMUM
} RageMemoryTier;

typedef struct RageMemoryBudget {
    int totalMiB, availableMiB;
    RageMemoryTier tier;
} RageMemoryBudget;

/* Available memory excludes swap; zero means the measurement is unknown. */
RageMemoryTier MemoryBudgetClassify(int totalMiB, int availableMiB);
const RageMemoryBudget *MemoryBudgetGet(void);
const char *MemoryBudgetName(RageMemoryTier tier);
int MemoryBudgetAllowHd(void);
int MemoryBudgetApplyConfig(RagePortConfig *config);
void MemoryBudgetStartWatch(void);
void MemoryBudgetStopWatch(void);
/* Called only at a presentation boundary. Event watches only set an atomic. */
int MemoryBudgetPoll(void);
#endif
