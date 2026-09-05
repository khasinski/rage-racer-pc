#ifndef MODERN_FRAME_PACER_H
#define MODERN_FRAME_PACER_H
#include <stdint.h>

/* Presentation has its own clock; never reset it at a PAL/NTSC logic tick. */
typedef struct ModernFramePacer { uint64_t next, interval; } ModernFramePacer;
static inline int ModernFrameDue(const ModernFramePacer *p, uint64_t now,
                                 uint64_t interval) {
    return !p->next || p->interval != interval || now >= p->next;
}
static inline void ModernFramePresented(ModernFramePacer *p, uint64_t now,
                                        uint64_t interval) {
    if (!interval) interval = 1;
    if (!p->next || p->interval != interval || now < p->next)
        p->next = now + interval;
    else
        p->next += ((now - p->next) / interval + 1) * interval;
    p->interval = interval;
}
#endif
