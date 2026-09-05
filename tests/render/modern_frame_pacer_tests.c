#include "modern_frame_pacer.h"
#include <stdio.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; } } while (0)
int main(void) {
    const int rates[] = {30, 60, 75, 120, 144, 240};
    for (int logic = 25; logic <= 30; logic += 5) {
        for (unsigned r = 0; r < sizeof(rates)/sizeof(rates[0]); ++r) {
            ModernFramePacer p = {0};
            uint64_t interval = 1000000000u / rates[r], last = 0;
            int frames = 0, ticks = 0;
            uint64_t tick = 0;
            for (uint64_t now = 0; now < 1000000000u; now += 100000u) {
                /* Logic events must not reset or force the presentation clock. */
                if (now >= tick) { ++ticks; tick += 1000000000u / logic; }
                if (ModernFrameDue(&p, now, interval)) {
                    if (frames) CHECK(now-last >= interval-100000u);
                    ModernFramePresented(&p, now, interval);
                    last = now;
                    ++frames;
                    CHECK(!ModernFrameDue(&p, now, interval));
                }
            }
            CHECK(frames == rates[r]);
            CHECK(ticks == logic);
        }
    }
    ModernFramePacer p = {0};
    ModernFramePresented(&p, 100, 10);
    CHECK(!ModernFrameDue(&p, 109, 10));
    CHECK(ModernFrameDue(&p, 110, 10));
    ModernFramePresented(&p, 195, 10); /* Missed frames are dropped, not burst. */
    CHECK(p.next == 200);
    CHECK(ModernFrameDue(&p, 196, 20)); /* Display/explicit FPS change. */
    ModernFramePresented(&p, 196, 20);
    CHECK(p.next == 216);
    return 0;
}
