#include "memory_budget.h"
#include "runtime_config.h"
#include <SDL3/SDL.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
/* Only memory-status declarations are needed. WinSock's u_long conflicts
 * with the pointer-sized legacy PSY-Q typedef on 64-bit Windows. */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

static RageMemoryBudget s_budget;
static int s_ready, s_watch, s_unrestricted;
static SDL_AtomicInt s_pressure;
static Uint64 s_nextProbe;

RageMemoryTier MemoryBudgetClassify(int total, int available) {
    if ((total > 0 && total <= 2048) || (available > 0 && available <= 512))
        return RAGE_MEMORY_MINIMUM;
    if ((total > 0 && total <= 4096) || (available > 0 && available <= 1024))
        return RAGE_MEMORY_LOW;
    return RAGE_MEMORY_NORMAL;
}

const char *MemoryBudgetName(RageMemoryTier tier) {
    return tier == RAGE_MEMORY_MINIMUM ? "minimum" :
           tier == RAGE_MEMORY_LOW ? "low" : "normal";
}

static int AvailableMiB(void) {
    const char *override = RuntimeConfigGet("diagnostics.memory_available_mb");
    int value;
    if (override && RuntimeParseInt(override, 10, 0, INT_MAX, &value)) return value;
#ifdef _WIN32
    MEMORYSTATUSEX status = {0};
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) {
        unsigned long long mib = status.ullAvailPhys / (1024u * 1024u);
        return mib > INT_MAX ? INT_MAX : (int)mib;
    }
#elif defined(__linux__) || defined(__ANDROID__)
    FILE *file = fopen("/proc/meminfo", "r");
    if (file) {
        char line[256];
        unsigned long long kib;
        while (fgets(line, sizeof(line), file)) {
            if (sscanf(line, "MemAvailable: %llu kB", &kib) == 1) {
                fclose(file);
                return kib / 1024u > INT_MAX ? INT_MAX : (int)(kib / 1024u);
            }
        }
        fclose(file);
    }
#endif
    return 0;
}

static void Report(const char *reason) {
    fprintf(stderr, "rage-port: memory total=%d MiB available=%d MiB profile=%s reason=%s\n",
            s_budget.totalMiB, s_budget.availableMiB, MemoryBudgetName(s_budget.tier), reason);
}

const RageMemoryBudget *MemoryBudgetGet(void) {
    if (!s_ready) {
        s_ready = 1;
        s_budget.totalMiB = SDL_GetSystemRAM();
        const char *override = RuntimeConfigGet("diagnostics.memory_total_mb");
        int value;
        if (override && RuntimeParseInt(override, 10, 0, INT_MAX, &value))
            s_budget.totalMiB = value;
        s_budget.availableMiB = AvailableMiB();
        s_budget.tier = MemoryBudgetClassify(s_budget.totalMiB, s_budget.availableMiB);
        const char *profile = RuntimeConfigGet("memory.profile");
        if (profile && !strcmp(profile, "normal")) {
            s_budget.tier = RAGE_MEMORY_NORMAL;
            s_unrestricted = 1;
        } else if (profile && !strcmp(profile, "low")) s_budget.tier = RAGE_MEMORY_LOW;
        else if (profile && !strcmp(profile, "minimum")) s_budget.tier = RAGE_MEMORY_MINIMUM;
        else if (profile && strcmp(profile, "auto"))
            fprintf(stderr, "rage-port: invalid memory.profile=%s; using auto\n", profile);
        Report("startup");
    }
    return &s_budget;
}

int MemoryBudgetAllowHd(void) { return MemoryBudgetGet()->tier == RAGE_MEMORY_NORMAL; }

int MemoryBudgetApplyConfig(RagePortConfig *config) {
    RageMemoryTier tier = MemoryBudgetGet()->tier;
    int changed = 0;
    if (!config || tier == RAGE_MEMORY_NORMAL) return 0;
    float scale = tier == RAGE_MEMORY_MINIMUM ? 1.5f : 2.0f;
    int fps = tier == RAGE_MEMORY_MINIMUM ? 30 : 60;
#define CAP(field, value) do { if (config->field > (value)) { config->field = (value); changed = 1; } } while (0)
    CAP(modernInternalScale, scale);
    CAP(modernDrawDistance, 1.0f);
    CAP(modernShadows, 0);
    CAP(modernSsao, 0);
    /* Vsync is unbounded on high-refresh displays; logic mode is already bounded. */
    if (config->modernFps == RAGE_MODERN_FPS_VSYNC || config->modernFps > fps) {
        config->modernFps = fps;
        changed = 1;
    }
#undef CAP
    return changed;
}

static bool SDLCALL OnEvent(void *context, SDL_Event *event) {
    (void)context;
    if (event->type == SDL_EVENT_LOW_MEMORY) SDL_SetAtomicInt(&s_pressure, 1);
    return true;
}

void MemoryBudgetStartWatch(void) {
    MemoryBudgetGet();
    if (!s_watch && SDL_AddEventWatch(OnEvent, NULL)) s_watch = 1;
}

void MemoryBudgetStopWatch(void) {
    if (s_watch) SDL_RemoveEventWatch(OnEvent, NULL);
    s_watch = 0;
}

int MemoryBudgetPoll(void) {
    MemoryBudgetGet();
    int pressure = SDL_SetAtomicInt(&s_pressure, 0);
    Uint64 now = SDL_GetTicksNS();
    RageMemoryTier next = s_budget.tier;
    if (s_unrestricted) return 0;
    if (pressure) next = RAGE_MEMORY_MINIMUM;
    else if (now >= s_nextProbe) {
        s_nextProbe = now + UINT64_C(5000000000);
        s_budget.availableMiB = AvailableMiB();
        next = MemoryBudgetClassify(s_budget.totalMiB, s_budget.availableMiB);
    }
    /* Do not oscillate quality or recreate resources when pressure recedes. */
    if (next <= s_budget.tier) return 0;
    s_budget.tier = next;
    Report(pressure ? "system low-memory event" : "available physical memory");
    return 1;
}
