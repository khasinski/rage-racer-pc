#include <psyz/audio.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;
#define CHECK(x) do { if (!(x)) { ++failures; \
    fprintf(stderr, "line %d: %s (%s)\n", __LINE__, #x, SDL_GetError()); \
} } while (0)

static int WaitForFrames(unsigned long long previous) {
    Uint64 deadline = SDL_GetTicks() + 1000;
    while (Psyz_AudioRenderedFrames() <= previous && SDL_GetTicks() < deadline)
        SDL_Delay(1);
    return Psyz_AudioRenderedFrames() > previous;
}

static void CheckExitCleanup(void) {
    unsigned long long stopped = Psyz_AudioRenderedFrames();
    SDL_Delay(60);
    CHECK(Psyz_AudioRenderedFrames() == stopped);
    Psyz_AudioDestroy();
    SDL_Quit();
    if (failures != 0) _Exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "exit-active") == 0) {
        /* Register first: this observer runs AFTER the backend's exit cleanup.
         * Leave playback running when returning from main. */
        CHECK(atexit(CheckExitCleanup) == 0);
        CHECK(SDL_SetHintWithPriority(SDL_HINT_AUDIO_DRIVER, "dummy",
                                     SDL_HINT_OVERRIDE));
        CHECK(Psyz_AudioInit() == 0);
        CHECK(WaitForFrames(0));
        return failures ? EXIT_FAILURE : EXIT_SUCCESS;
    }
    /* An actual unavailable backend exercises partial initialization and retry.
     * Hints override the host configuration; no audible device is required. */
    CHECK(SDL_SetHintWithPriority(SDL_HINT_AUDIO_DRIVER,
                                 "rage-nonexistent-audio-driver",
                                 SDL_HINT_OVERRIDE));
    Psyz_AudioDestroy();
    CHECK(Psyz_AudioInit() == -1);
    Psyz_AudioDestroy();
    Psyz_AudioDestroy();
    CHECK(SDL_SetHintWithPriority(SDL_HINT_AUDIO_DRIVER, "dummy",
                                 SDL_HINT_OVERRIDE));
    for (int pass = 0; pass < 40; ++pass) {
        Psyz_AudioResetMetrics();
        int initialized = Psyz_AudioInit();
        CHECK(initialized == 0);
        if (initialized != 0) break;
        CHECK(Psyz_AudioInit() == 0);
        CHECK(WaitForFrames(0));
        Psyz_AudioPause();
        /* Synchronize with any callback already in flight at pause time. */
        Psyz_AudioLock();
        unsigned long long paused = Psyz_AudioRenderedFrames();
        Psyz_AudioUnlock();
        SDL_Delay(30);
        CHECK(Psyz_AudioRenderedFrames() == paused);
        Psyz_AudioUnpause();
        CHECK(WaitForFrames(paused));
        /* Destroy while running, rather than making teardown safe by pausing. */
        Psyz_AudioDestroy();
        unsigned long long stopped = Psyz_AudioRenderedFrames();
        SDL_Delay(30);
        CHECK(Psyz_AudioRenderedFrames() == stopped);
        Psyz_AudioDestroy();
        /* Exercise both reuse of SDL audio and its independent shutdown. */
        if (pass % 2 == 0) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
    Psyz_AudioDestroy();
    SDL_Quit();
    if (failures == 0) puts("audio lifecycle: failed init/retry and 40 cycles passed");
    return failures ? 1 : 0;
}
