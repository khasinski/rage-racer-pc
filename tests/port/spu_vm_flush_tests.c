#include <stdio.h>

int _snd_ev_flag;
static int s_flushCalls;
static int s_guardViolations;

void SpuVmDamperStep(void);

void _SsVmFlush(void) {
    s_flushCalls++;
    if (_snd_ev_flag != 1) {
        s_guardViolations++;
    }
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    _snd_ev_flag = 0;
    s_flushCalls = 0;
    s_guardViolations = 0;
    SpuVmDamperStep();
    CHECK(s_flushCalls == 1 && s_guardViolations == 0 && _snd_ev_flag == 0);

    _snd_ev_flag = 1;
    SpuVmDamperStep();
    CHECK(s_flushCalls == 1 && _snd_ev_flag == 1);
    puts("SPU flush guard prevents nested sound event processing");
    return 0;
}
