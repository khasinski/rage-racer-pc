/* Physics parity check: drives the web bridge with a fixed input script and
 * prints the local car's exact state. Built natively (native_stub) and
 * compared against web/scripts/parity.mjs running the WebAssembly build;
 * both must print identical lines. */
#include <stdint.h>
#include <stdio.h>

int rw_load_disc(const char *path);
int rw_start_race(int classIndex, int course, int car, int manual, int reverse, int laps, int rivals);
void rw_set_input(int left, int right, int throttle, int brake, int shiftUp, int shiftDown);
int rw_tick(void);
int32_t *rw_hud(void);

/* Keep in step with web/scripts/parity.mjs. */
static void ScriptedInput(int tick, int *left, int *right, int *throttle, int *brake) {
    *left = tick >= 300 && tick < 340;
    *right = (tick >= 400 && tick < 460) || (tick >= 700 && tick < 720);
    *brake = tick >= 520 && tick < 545 ? 256 : 0;
    *throttle = *brake ? 0 : 256;
}

int main(int argc, char **argv) {
    int ticks = 1000;
    if (argc < 2) { fprintf(stderr, "usage: parity <Track 01 BIN> [ticks]\n"); return 2; }
    if (argc > 2) sscanf(argv[2], "%d", &ticks);
    if (!rw_load_disc(argv[1]) || !rw_start_race(0, 0, 9, 0, 0, 3, 1)) return 1;
    for (int tick = 1; tick <= ticks; ++tick) {
        int left, right, throttle, brake;
        ScriptedInput(tick, &left, &right, &throttle, &brake);
        rw_set_input(left, right, throttle, brake, 0, 0);
        if (rw_tick() < 0) return 1;
        if (tick % 10 == 0) {
            const int32_t *h = rw_hud();
            printf("%d phase=%d lap=%d x=%d y=%d z=%d yaw=%d speed=%d gear=%d time=%d\n",
                   tick, h[0], h[2], h[12], h[13], h[14], h[15], h[7], h[8], h[6]);
        }
    }
    return 0;
}
