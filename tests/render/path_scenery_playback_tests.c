#include "common.h"
#include "game/audio.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/track_internal.h"

#include <stdio.h>
#include <string.h>

void InitPathScenery(void);
void UpdatePathScenerySound(void);

s32 g_RaceSeries;
s16 g_RacePhase;
PlayerCarRuntime g_PlayerCar;
PathSceneryPositionData *g_PathSceneryPosData;
PathSceneryRotationData *g_PathSceneryRotData;
PathSceneryPositionKey *g_PathSceneryPosKeys;
PathSceneryRotationKey *g_PathSceneryRotKeys;
PathSceneryClock g_PathSceneryClock;
PathSceneryTransform g_PathSceneryTransform;
PathSceneryCursors g_PathSceneryCursors;
s16 g_PathSceneryHalfDelta[3];
s16 g_PathSceneryRotHalfDelta[3];
s32 g_PathSceneryVolume;

static s32 g_LastCue;
static s32 g_LastPitch;
static s32 g_LastVolume;

void SetPitchedSoundCue(s32 cue, s32 pitch, s32 volume) {
    g_LastCue = cue;
    g_LastPitch = pitch;
    g_LastVolume = volume;
}

typedef struct PositionFixture {
    s16 firstKey[2];
    PathSceneryPositionKey keys[3];
} PositionFixture;

typedef struct RotationFixture {
    s16 firstKey[2];
    PathSceneryRotationKey keys[3];
} RotationFixture;

static u32 FoldWord(u32 digest, s32 value) {
    int byte;

    for (byte = 0; byte < 4; byte++) {
        digest ^= ((u32)value >> (byte * 8)) & 0xFF;
        digest *= 16777619U;
    }
    return digest;
}

int main(void) {
    PositionFixture positions;
    RotationFixture rotations;
    u32 digest = 2166136261U;
    static const u32 expected = 3752343587U;
    int frame;

    memset(&positions, 0, sizeof(positions));
    memset(&rotations, 0, sizeof(rotations));
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));

    positions.keys[0].fields.span = 3;
    positions.keys[0].fields.rate = 2;
    positions.keys[1].fields.x = 100;
    positions.keys[1].fields.y = 50;
    positions.keys[1].fields.z = -100;
    positions.keys[1].fields.span = 2;
    positions.keys[1].fields.rate = -2;
    positions.keys[2].fields.span = -1;
    positions.keys[2].fields.loopIndex = 0;

    rotations.keys[0].fields.span = 3;
    rotations.keys[0].fields.rate = 2;
    rotations.keys[1].fields.x = 200;
    rotations.keys[1].fields.y = -100;
    rotations.keys[1].fields.z = 300;
    rotations.keys[1].fields.span = 2;
    rotations.keys[1].fields.rate = -2;
    rotations.keys[2].fields.span = -1;
    rotations.keys[2].fields.loopIndex = 0;

    g_PathSceneryPosData = (PathSceneryPositionData *)&positions;
    g_PathSceneryRotData = (PathSceneryRotationData *)&rotations;
    g_PlayerCar.x = 120;
    g_PlayerCar.y = 40;
    g_PlayerCar.z = -80;
    InitPathScenery();

    for (frame = 0; frame < 12; frame++) {
        g_RacePhase = frame < 10 ? 2 : 3;
        UpdatePathScenerySound();
        if (frame == 3 &&
            (g_PathSceneryCursors.rotRate.signedValue != 2 ||
             g_PathSceneryTransform.rotation.vx != 200 ||
             g_PathSceneryTransform.rotation.vy != -100 ||
             g_PathSceneryTransform.rotation.vz != 300)) {
            puts("FAIL: negative rotation rate was not normalized");
            return 1;
        }
        digest = FoldWord(digest, g_PathSceneryClock.posFrame);
        digest = FoldWord(digest, g_PathSceneryClock.rotFrame);
        digest = FoldWord(digest, g_PathSceneryCursors.posIndex);
        digest = FoldWord(digest, g_PathSceneryCursors.rotIndex);
        digest = FoldWord(digest, g_PathSceneryCursors.posPhase.signedValue);
        digest = FoldWord(digest, g_PathSceneryCursors.rotPhase.signedValue);
        digest = FoldWord(digest, g_PathSceneryTransform.position.w[0]);
        digest = FoldWord(digest, g_PathSceneryTransform.position.w[1]);
        digest = FoldWord(digest, g_PathSceneryTransform.position.w[2]);
        digest = FoldWord(digest, g_PathSceneryTransform.rotation.vx);
        digest = FoldWord(digest, g_PathSceneryTransform.rotation.vy);
        digest = FoldWord(digest, g_PathSceneryTransform.rotation.vz);
        digest = FoldWord(digest, g_PathSceneryVolume);
        digest = FoldWord(digest, g_LastCue);
        digest = FoldWord(digest, g_LastPitch);
        digest = FoldWord(digest, g_LastVolume);
    }

    if (digest != expected) {
        printf("FAIL: path scenery digest %u, expected %u\n", digest,
               expected);
        return 1;
    }
    puts("path scenery playback preserved");
    return 0;
}
