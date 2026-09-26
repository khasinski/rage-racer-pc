#include "game/car_internal.h"
#include "game/car_motion_internal.h"
#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
s32 g_MirrorMode;
static int calls;
static s32 cue;
void PlaySoundCue(s32 value) { cue = value; calls++; }

int main(void) {
    PlayPlayerLandingCue(19, 0);
    PlayPlayerLandingCue(18, 1);
    CHECK(calls == 0);
    PlayPlayerLandingCue(19, 1);
    CHECK(calls == 1 && cue == 0xE);
    PlayerCarRuntime car = {.motionTimer = 15, .speed = 81};
    const PlayerCarRuntime saved = car;
    for (int mirror = 0; mirror < 2; mirror++) {
        g_MirrorMode = mirror;
        for (int skid = CAR_TRACK_CONTACT_FRONT_LEFT; skid <= CAR_TRACK_CONTACT_REAR_RIGHT; skid++) {
            calls = 0;
            PlayPlayerContactCue(&car, skid, 512, 0);
            CHECK(calls == 0);
            PlayPlayerContactCue(&car, skid, -1, 1);
            CHECK(calls == 0);
            PlayPlayerContactCue(&car, skid, 512, 1);
            const int near = skid == CAR_TRACK_CONTACT_FRONT_LEFT || skid == CAR_TRACK_CONTACT_REAR_LEFT;
            CHECK(calls == 1 && cue == ((near != mirror) ? 0xB : 0xC));
            calls = 0;
            PlayPlayerContactCue(&car, skid, 768, 1);
            CHECK(calls == 1 && cue == (skid <= CAR_TRACK_CONTACT_FRONT_RIGHT ? 0xA : 0xD));
        }
    }
    CHECK(memcmp(&car, &saved, sizeof(car)) == 0);
    calls = 0;
    car.motionTimer = 14;
    PlayPlayerContactCue(&car, CAR_TRACK_CONTACT_FRONT_LEFT, 768, 1);
    CHECK(calls == 0);
    car.motionTimer = 15;
    car.speed = 80;
    PlayPlayerContactCue(&car, CAR_TRACK_CONTACT_REAR_LEFT, 768, 1);
    CHECK(calls == 0);
    return 0;
}
