#include <psyz.h>
#include <libcd.h>
#include <psyz/cd.h>
#include <psyz/audio.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(x) do { if (!(x)) { ++failures; \
    fprintf(stderr, "line %d: %s\n", __LINE__, #x); } } while (0)
typedef struct Source { unsigned calls; unsigned char sample; } Source;
static int Read(unsigned sector, void *buffer, void *user) {
    Source *source = user;
    (void)sector;
    ++source->calls;
    unsigned char *bytes = buffer;
    for (unsigned i = 0; i < 2352; i += 2) {
        bytes[i] = source->sample;
        bytes[i + 1] = 0;
    }
    return 2352;
}
static int Mount(Source *source) {
    PsyzCdTrackInfo track = {0, 100, 1, 0};
    return Psyz_CdSetSectorBackend(&track, 1, 100, Read, source);
}
static void Play(void) {
    unsigned char mode = CdlModeDA;
    CdlLOC location = {0};
    CdControl(CdlSetmode, &mode, NULL);
    CdIntToPos(0, &location);
    CdControl(CdlSetloc, (unsigned char *)&location, NULL);
    CdControl(CdlPlay, NULL, NULL);
}
static int ReadXa(unsigned sector, void *buffer, void *user) {
    Source *source = user;
    unsigned char *bytes = buffer;
    (void)sector;
    ++source->calls;
    memset(bytes, 0, 2352);
    bytes[15] = 2;
    /* Distinct file ID and ADPCM nibbles exercise old filter/history state. */
    bytes[16] = source->sample;
    bytes[18] = 0x64;
    bytes[19] = 1; /* stereo, 37800 Hz, 4-bit */
    memcpy(bytes + 20, bytes + 16, 4);
    for (unsigned block = 0; block < 18; ++block)
        memset(bytes + 24 + block * 128 + 16,
               source->sample | (source->sample << 4), 112);
    return 2352;
}
static int MountXa(Source *source) {
    PsyzCdTrackInfo track = {0, 4, 0, 0};
    return Psyz_CdSetSectorBackend(&track, 1, 4, ReadXa, source);
}
static void PlayXa(void) {
    unsigned char mode = CdlModeRT | CdlModeSF;
    CdlLOC location = {0};
    CdControl(CdlSetmode, &mode, NULL);
    CdIntToPos(0, &location);
    CdControl(CdlSetloc, (unsigned char *)&location, NULL);
    CdControl(CdlReadN, NULL, NULL);
}
static void CheckXaReplacement(void) {
    Source first = {0, 1}, second = {0, 3};
    short baseline[6000 * 2] = {0}, actual[6000 * 2] = {0};
    CHECK(MountXa(&second) == 0);
    PlayXa();
    CHECK(Psyz_CdPullSamples(baseline, 6000) == 6000);
    CHECK(baseline[200] != 0);
    for (unsigned cycle = 0; cycle < 10; ++cycle) {
        CHECK(MountXa(&first) == 0);
        Psyz_CdSetXaEndSector(1);
        unsigned char filter[2] = {1, 0};
        CdControl(CdlSetfilter, filter, NULL);
        PlayXa();
        CHECK(Psyz_CdPullSamples(actual, 64) == 64);
        unsigned calls = first.calls;
        CHECK(MountXa(&second) == 0);
        CHECK(!Psyz_CdAudioPlaying());
        CHECK(Psyz_CdPullSamples(actual, 64) == 0);
        PlayXa(); /* no new filter/limit: old disc must not constrain this one */
        memset(actual, 0, sizeof(actual));
        CHECK(Psyz_CdPullSamples(actual, 6000) == 6000);
        CHECK(memcmp(actual, baseline, sizeof(actual)) == 0);
        CHECK(first.calls == calls);
        CHECK(Psyz_CdSetDiskPath(NULL) == 0);
        CHECK(!Psyz_CdAudioPlaying());
        CHECK(Psyz_CdPullSamples(actual, 64) == 0);
    }
}
typedef struct ConcurrentSource {
    SDL_AtomicInt calls, retired, violations, active;
} ConcurrentSource;
static int ReadConcurrent(unsigned sector, void *buffer, void *user) {
    ConcurrentSource *source = user;
    (void)sector;
    if (SDL_GetAtomicInt(&source->retired)) SDL_AddAtomicInt(&source->violations, 1);
    SDL_AddAtomicInt(&source->active, 1);
    SDL_AddAtomicInt(&source->calls, 1);
    /* Give the lifecycle thread a window to request unmount while a read is
     * in flight. It must wait for this callback before retiring its owner. */
    SDL_Delay(2);
    memset(buffer, 0x12, 2352);
    if (SDL_GetAtomicInt(&source->retired)) SDL_AddAtomicInt(&source->violations, 1);
    SDL_AddAtomicInt(&source->active, -1);
    return 2352;
}
static void CheckConcurrentUnmount(void) {
    ConcurrentSource sources[20] = {0};
    unsigned overlaps = 0;
    unsigned long long initialFrames = Psyz_AudioRenderedFrames();
    Psyz_AudioUnpause();
    for (unsigned cycle = 0; cycle < 20; ++cycle) {
        ConcurrentSource *source = &sources[cycle];
        PsyzCdTrackInfo track = {0, 100, 1, 0};
        CHECK(Psyz_CdSetSectorBackend(&track, 1, 100, ReadConcurrent, source) == 0);
        Psyz_AudioLock();
        Play();
        Psyz_AudioUnlock();
        Uint64 deadline = SDL_GetTicks() + 1000;
        while (SDL_GetAtomicInt(&source->calls) == 0 && SDL_GetTicks() < deadline)
            SDL_Delay(1);
        CHECK(SDL_GetAtomicInt(&source->calls) > 0);
        if (SDL_GetAtomicInt(&source->active) > 0) ++overlaps;
        CHECK(Psyz_CdSetDiskPath(NULL) == 0);
        CHECK(SDL_GetAtomicInt(&source->active) == 0);
        SDL_SetAtomicInt(&source->retired, 1);
        int calls = SDL_GetAtomicInt(&source->calls);
        SDL_Delay(3);
        CHECK(SDL_GetAtomicInt(&source->calls) == calls);
        CHECK(!Psyz_CdAudioPlaying());
    }
    Psyz_AudioPause();
    CHECK(Psyz_AudioRenderedFrames() > initialFrames);
    CHECK(overlaps > 0);
    CHECK(Psyz_CdSetDiskPath(NULL) == 0);
    /* Keep all owners allocated throughout so a late access is a deterministic
     * test failure, not an uncontrolled use-after-free in the test itself. */
    for (unsigned i = 0; i < 20; ++i)
        CHECK(SDL_GetAtomicInt(&sources[i].violations) == 0);
}
int main(void) {
    Source first = {0, 0x12}, second = {0, 0x34};
    short samples[32] = {0};
    SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
    if (Psyz_AudioInit() != 0) return 1;
    Psyz_AudioPause();
    /* Pull the real CD backend synchronously, with its host stream paused. */
    for (unsigned cycle = 0; cycle < 20; ++cycle) {
        CHECK(Mount(&first) == 0);
        Play();
        CHECK(Psyz_CdPullSamples(samples, 16) == 16);
        CHECK(samples[0] == 0x12);
        unsigned calls = first.calls;
        CHECK(Psyz_CdSetDiskPath(NULL) == 0);
        CHECK(!Psyz_CdAudioPlaying());
        CHECK(Psyz_CdPullSamples(samples, 16) == 0);
        CHECK(first.calls == calls);
        CHECK(Psyz_CdAudioFramesPulled() == 0);
        CHECK(Psyz_CdAudioEnergy() == 0);
        CHECK(Psyz_CdGetTrackSector(1) == -1);
        CHECK(Psyz_CdSetDiskPath(NULL) == 0);
        CHECK(Mount(&second) == 0);
        CHECK(!Psyz_CdAudioPlaying());
        CHECK(Psyz_CdPullSamples(samples, 16) == 0);
        Play();
        CHECK(Psyz_CdPullSamples(samples, 16) == 16);
        CHECK(samples[0] == 0x34); /* no previous source's prefetched samples */
        CHECK(Mount(&first) == 0); /* replace without a separate unmount */
        CHECK(!Psyz_CdAudioPlaying());
        CHECK(Psyz_CdPullSamples(samples, 16) == 0);
        Play();
        CHECK(Psyz_CdPullSamples(samples, 16) == 16);
        CHECK(samples[0] == 0x12);
        CHECK(Psyz_CdSetSectorBackend(NULL, 0, 0, NULL, NULL) == -1);
        CHECK(!Psyz_CdAudioPlaying());
        CHECK(Psyz_CdPullSamples(samples, 16) == 0);
        CHECK(Mount(&second) == 0);
        Play();
        CHECK(Psyz_CdSetDiskPath("") == -1);
        CHECK(!Psyz_CdAudioPlaying());
        CHECK(Psyz_CdPullSamples(samples, 16) == 0);
    }
    CheckXaReplacement();
    CheckConcurrentUnmount();
    CHECK(Psyz_CdSetDiskPath(NULL) == 0);
    Psyz_AudioDestroy();
    SDL_Quit();
    return failures != 0;
}
