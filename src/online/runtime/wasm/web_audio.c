/* Race audio for the browser (see web_audio.h).
 *
 * The sound calls are the desktop's own retail code, copied into web/engine:
 * the engine layers (audio/engine_sound_runtime.c), the cue tables and voice
 * management (audio/sound_cues.c, effect voices, stereo/pitched cues) and the
 * player car's tyre, jump and contact sounds (the car/ audio files). This file does
 * what the retail race scene does around them (race/race_scene.c,
 * car/update_player_car.c, race/lap_and_finish.c, track ambience), reading the
 * race simulation instead of the retail globals, once per 25 Hz game frame. */
#include "web_audio.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "game/angle.h"
#include "game/asset_index.h"
#include "game/audio.h"
#include "game/audio_internal.h"
#include "game/audio_state_internal.h"
#include "game/car_asset.h"
#include "game/car_audio.h"
#include "game/engine_sound.h"
#include "game/race.h"
#include "game/sound.h"
#include "game/track.h"
#include "psyq/snd.h"
#include "web_spu.h"

/* ---- retail state the copied audio code reads ------------------------------ */

/* The browser never mirrors a course. */
s32 g_MirrorMode;
SoundScale g_SoundScale;

/* src/port/native_initialized_state.c */
SoundCueParams g_SoundCueParams[MAIN_SOUND_CUE_COUNT] = {
    {128, 0, 0, 0, 0, 60}, {86, 0, 1, 0, 1, 60},  {96, 0, 2, 0, 1, 60},   {84, 0, 3, 0, 1, 60},
    {80, 0, 4, 0, 1, 60},  {90, 0, 5, 0, 1, 60},  {98, 0, 8, 0, 1, 60},   {128, 0, 0, 0, 0, 60},
    {98, 0, 8, 0, 1, 60},  {128, 0, 0, 0, 0, 60}, {128, 0, 0, 0, 0, 60},  {128, 0, 0, 0, 0, 60},
    {128, 0, 0, 0, 0, 60}, {128, 0, 0, 0, 0, 60}, {128, 0, 0, 0, 0, 60},  {110, 0, 23, 0, 1, 60},
    {95, 0, 24, 0, 1, 60}, {105, 0, 25, 0, 1, 60}, {128, 0, 0, 0, 0, 60}, {128, 0, 0, 0, 0, 60},
    {128, 0, 0, 0, 0, 60}, {128, 0, 0, 0, 0, 60}, {128, 0, 0, 0, 0, 60},  {128, 0, 0, 0, 0, 60},
    {128, 0, 0, 0, 0, 60}, {110, 0, 6, 0, 1, 60}, {110, 0, 7, 0, 1, 60},  {128, 0, 0, 0, 0, 60},
    {128, 0, 0, 0, 0, 60}, {128, 0, 0, 0, 0, 60},
};
SoundCueParams g_SoundCueParams2[RACE_SOUND_CUE_COUNT] = {
    {128, 0, 0, 0, 0, 60},  {86, 0, 1, 0, 1, 60},   {96, 0, 2, 0, 1, 60},   {84, 0, 3, 0, 1, 60},
    {80, 0, 0, 0, 0, 60},   {90, 0, 5, 0, 1, 60},   {98, 0, 8, 0, 1, 60},   {128, 0, 0, 0, 0, 60},
    {98, 0, 8, 0, 1, 60},   {128, 0, 0, 0, 0, 60},  {128, 0, 10, 0, 0, 60}, {128, 0, 11, 0, 0, 60},
    {128, 0, 12, 0, 0, 60}, {128, 0, 13, 0, 0, 60}, {128, 0, 9, 0, 0, 60},  {110, 0, 23, 0, 1, 60},
    {95, 0, 24, 0, 1, 60},  {105, 0, 25, 0, 1, 60}, {128, 0, 0, 0, 0, 60},  {128, 0, 0, 0, 0, 60},
    {128, 3, 10, 0, 0, 60}, {128, 0, 0, 0, 0, 60},  {128, 0, 0, 0, 0, 60},  {128, 0, 0, 0, 0, 60},
    {128, 0, 0, 0, 0, 60},  {110, 0, 6, 0, 1, 60},  {110, 0, 7, 0, 1, 60},  {128, 0, 0, 0, 0, 60},
    {128, 0, 0, 0, 0, 60},  {128, 0, 0, 0, 0, 60},  {120, 2, 1, 0, 1, 60},  {120, 2, 2, 0, 1, 60},
    {120, 2, 3, 0, 1, 60},  {120, 2, 4, 0, 1, 60},  {120, 2, 5, 0, 1, 60},  {120, 2, 6, 0, 1, 60},
    {120, 2, 6, 0, 1, 60},  {120, 2, 6, 0, 1, 60},  {120, 2, 7, 0, 1, 60},  {120, 2, 8, 0, 1, 60},
    {120, 2, 9, 0, 1, 60},  {120, 2, 10, 0, 1, 60}, {120, 2, 11, 0, 1, 60}, {120, 2, 12, 0, 1, 60},
    {120, 2, 13, 0, 1, 60}, {120, 2, 14, 0, 1, 60}, {120, 2, 14, 0, 1, 60}, {120, 2, 15, 0, 1, 60},
    {120, 2, 16, 0, 1, 60}, {120, 2, 15, 0, 1, 60}, {120, 2, 27, 0, 1, 60}, {120, 2, 17, 0, 1, 60},
    {120, 2, 28, 0, 1, 60}, {120, 2, 0, 0, 1, 60},  {120, 2, 18, 0, 1, 60}, {120, 2, 19, 0, 1, 60},
    {120, 2, 0, 0, 1, 60},  {120, 2, 0, 0, 1, 60},  {120, 2, 0, 0, 1, 60},  {120, 2, 0, 0, 1, 60},
    {120, 2, 0, 0, 1, 60},  {110, 2, 21, 0, 1, 60}, {120, 2, 22, 0, 1, 60}, {120, 2, 23, 0, 1, 60},
    {120, 2, 24, 0, 1, 60}, {120, 2, 25, 0, 1, 60}, {120, 2, 26, 0, 1, 60}, {120, 2, 0, 0, 1, 60},
    {120, 2, 0, 0, 1, 60},  {120, 2, 0, 0, 1, 60},
};
EffectCueBank g_EffectCueTable[EFFECT_CUE_BANK_COUNT] = {
    {2, 85, {{22, 0}, {22, 0}}},
    {2, 118, {{17, 0}, {17, 1}}},
    {2, 128, {{26, 0}, {26, 1}}},
};
s32 g_SpecialVoiceBits[SPECIAL_VOICE_BIT_COUNT] = {
    0x00040000, 0x00080000, 0x00100000, 0x00200000, 0x00400000, 0x00800000,
};

/* ---- disc assets ------------------------------------------------------------ */

enum {
    ASSET_BOOT_AUDIO_HEADER = 2, /* include/game/asset.h */
    ASSET_BOOT_AUDIO_BODY = 3,
    DEFAULT_SFX_SETTING = 15,    /* save/save_defaults.c */
    CUE_BANK_RACE = 2,           /* PollAudioSlotLoad after the race banks */
    /* Browser voices for the other human cars' engines: four layers each. */
    OTHER_ENGINE_FIRST_VOICE = WEB_SPU_HARDWARE_VOICES,
    OTHER_ENGINE_LAYERS = 4,
    OTHER_ENGINE_CARS = (WEB_SPU_VOICES - WEB_SPU_HARDWARE_VOICES) / OTHER_ENGINE_LAYERS,
    OTHER_ENGINE_FIRST_VAB = AUDIO_SLOT_COUNT,
    PENDING_CAPACITY = WEB_AUDIO_FRAMES_PER_TICK * 50, /* one second */
};

/* audio/audio_initialization.c: engine loudness per car asset, which
 * InitEffectVoiceRuntime puts in place of the car table's own scale. */
static const s32 s_carVolumeScales[CAR_SOUND_VOLUME_SCALE_COUNT] = {
    20, 21, 22, 23, 21, 22, 23, 22, 23, 26, 27, 28, 29, 30, 50, 52,
    54, 50, 52, 54, 52, 42, 44, 28, 28, 29, 30, 31, 30, 26, 46, 80,
};

static s32 ReadS32(const uint8_t *bytes) {
    return (s32)((u32)bytes[0] | (u32)bytes[1] << 8 | (u32)bytes[2] << 16 | (u32)bytes[3] << 24);
}

/* The car's second asset: specification, engine VAB header, engine
 * parameter table, VAB body, then its image (race_assets.c). */
typedef struct CarAudio {
    const uint8_t *header, *table, *body;
    size_t headerSize, tableSize, bodySize;
} CarAudio;

static int ReadCarAudio(const RaceData *archive, s32 variant, CarAudio *out) {
    size_t size;
    const uint8_t *pack = RaceAsset(archive, CarVariantAssetIndex(ASSET_CAR_2ND_BASE, variant), &size);
    s32 header, table, body, image;
    if (!pack || size < sizeof(RaceCarAssetHeader)) return 0;
    header = ReadS32(pack + offsetof(RaceCarAssetHeader, audioHeaderOffset));
    table = ReadS32(pack + offsetof(RaceCarAssetHeader, audioSequenceOffset));
    body = ReadS32(pack + offsetof(RaceCarAssetHeader, audioBodyOffset));
    image = ReadS32(pack + offsetof(RaceCarAssetHeader, imageOffset));
    if (header <= 0 || table <= header || body <= table || image <= body || (size_t)image > size ||
        (size_t)(body - table) < ENGINE_SOUND_PARAMETER_TABLE_SIZE) return 0;
    *out = (CarAudio){pack + header, pack + table, pack + body,
                      (size_t)(table - header), (size_t)(body - table), (size_t)(image - body)};
    return 1;
}

static int OpenBootBank(const RaceData *archive) {
    size_t headerSize, bodySize;
    const uint8_t *header = RaceAsset(archive, ASSET_BOOT_AUDIO_HEADER, &headerSize);
    const uint8_t *body = RaceAsset(archive, ASSET_BOOT_AUDIO_BODY, &bodySize);
    return header && body && WebSpuOpenVab(AUDIO_SLOT_MAIN_CUES, header, headerSize, body, bodySize) >= 0;
}

/* round_screen_assets.c LoadRoundVoiceBank: the announcer. */
static int OpenVoiceBank(const RaceData *archive) {
    size_t size;
    const uint8_t *pack = RaceAsset(archive, ASSET_VOICE_BANK, &size);
    s32 headerSize, headerOffset, bodyOffset;
    if (!pack || size < 12) return 0;
    headerSize = ReadS32(pack);
    headerOffset = ReadS32(pack + 4);
    bodyOffset = ReadS32(pack + 8);
    if (headerSize < 0 || headerOffset < 12 || (size_t)headerOffset > size ||
        (size_t)headerSize > size - (size_t)headerOffset || bodyOffset != headerOffset + headerSize ||
        (size_t)bodyOffset >= size) return 0;
    return WebSpuOpenVab(AUDIO_SLOT_RACE_CUES, pack + headerOffset, (size_t)headerSize,
                         pack + bodyOffset, size - (size_t)bodyOffset) >= 0;
}

/* ---- the other human cars' engines -------------------------------------------
 * The retail game only ever sounds the player's engine. Online the browser
 * also plays the other drivers' own engine banks, through the same curves,
 * attenuated and panned by where they are relative to the local car. */

typedef struct OtherEngine {
    int seat, vab, bank, keyed;
    EngineSound sound;
    EngineSoundCurveRow curves[ENGINE_SOUND_BANK_COUNT][ENGINE_SOUND_PARAMETER_COUNT];
    s16 tones[ENGINE_SOUND_SLOT_COUNT][ENGINE_SOUND_BANK_COUNT];
    s32 maxRpm, volumeScale;
} OtherEngine;

typedef struct PlayerEngineTable {
    EngineSoundCurveRow curves[ENGINE_SOUND_BANK_COUNT][ENGINE_SOUND_PARAMETER_COUNT];
    s16 tones[ENGINE_SOUND_SLOT_COUNT][ENGINE_SOUND_BANK_COUNT];
    s32 maxRpm;
} PlayerEngineTable;

static void SaveEngineTable(PlayerEngineTable *table) {
    memcpy(table->curves, g_EngineSoundCurves, sizeof(table->curves));
    memcpy(table->tones, g_SoundSlotTone, sizeof(table->tones));
    table->maxRpm = g_EngineSoundState.maxRpm;
}

static void RestoreEngineTable(const PlayerEngineTable *table) {
    memcpy(g_EngineSoundCurves, table->curves, sizeof(table->curves));
    memcpy(g_SoundSlotTone, table->tones, sizeof(table->tones));
    g_EngineSoundState.maxRpm = table->maxRpm;
}

/* ---- race state --------------------------------------------------------------- */

static struct {
    int active, enabled, seat, classIndex, variant;
    u32 lastTick, lastStepTick, frame;
    SimRacePhase phase;
    SimDriverStatus status;
    s32 lap, motionState, countdownCue;
    s32 raceCueFlags, raceCueDelay, finishFollowupCue, finishFollowupWait, wrongWayTimer;
    s32 bestLap;
    EngineSound engine;
    OtherEngine others[OTHER_ENGINE_CARS];
    int otherCount;
    int16_t pending[PENDING_CAPACITY * 2];
    int pendingFrames;
} s;

static void StopEffects(void) { ForceAllEffectVoicesEnabled(0); }

static void OpenOtherEngines(const RaceData *archive, const RaceSim *race) {
    PlayerEngineTable player;
    SaveEngineTable(&player);
    s.otherCount = 0;
    for (int seat = 0; seat < DRIVER_SEAT_LIMIT && s.otherCount < OTHER_ENGINE_CARS; ++seat) {
        const SimDriver *driver = &race->drivers[seat];
        OtherEngine *other = &s.others[s.otherCount];
        CarAudio audio;
        if (seat == s.seat || driver->status == SIM_EMPTY || driver->rival || driver->variant < 0 ||
            !ReadCarAudio(archive, driver->variant, &audio)) continue;
        memset(other, 0, sizeof(*other));
        other->seat = seat;
        other->vab = OTHER_ENGINE_FIRST_VAB + s.otherCount;
        if (WebSpuOpenVab(other->vab, audio.header, audio.headerSize, audio.body, audio.bodySize) < 0) continue;
        LoadAudioParameterTable(audio.table, audio.tableSize);
        memcpy(other->curves, g_EngineSoundCurves, sizeof(other->curves));
        memcpy(other->tones, g_SoundSlotTone, sizeof(other->tones));
        other->maxRpm = g_EngineSoundState.maxRpm;
        other->volumeScale = s_carVolumeScales[(u32)driver->variant < CAR_SOUND_VOLUME_SCALE_COUNT ? driver->variant : 0];
        other->bank = -1;
        ++s.otherCount;
    }
    RestoreEngineTable(&player);
}

int WebAudioStartRace(const RaceData *archive, const RaceSim *race, int localSeat, int classIndex) {
    const SimDriver *driver;
    CarAudio audio;
    WebAudioStopRace();
    if (!archive || !race || localSeat < 0 || localSeat >= DRIVER_SEAT_LIMIT) return 0;
    driver = &race->drivers[localSeat];
    memset(&s.engine, 0, sizeof(s.engine));
    s.seat = localSeat;
    s.classIndex = classIndex;
    s.variant = driver->variant;
    if (driver->variant < 0 || !OpenBootBank(archive) || !OpenVoiceBank(archive) ||
        !ReadCarAudio(archive, driver->variant, &audio) ||
        WebSpuOpenVab(AUDIO_SLOT_ENGINE, audio.header, audio.headerSize, audio.body, audio.bodySize) < 0) {
        WebSpuReset();
        return 0;
    }
    for (int slot = 0; slot < SOUND_SCALE_VAB_ID_CAPACITY; ++slot) g_SoundScale.vabIds[slot] = (s16)slot;
    InitSoundRuntime();
    SetEffectVolumeSetting(DEFAULT_SFX_SETTING);
    LoadAudioParameterTable(audio.table, audio.tableSize);
    g_Audio.slots.loaded = 1 << AUDIO_SLOT_MAIN_CUES | 1 << AUDIO_SLOT_RACE_CUES | 1 << AUDIO_SLOT_ENGINE;
    g_Audio.slots.cueBank = CUE_BANK_RACE;
    /* race_scene.c's scene entry: InitEffectVoiceRuntime. */
    SetSoundSlotVoicesEnabled(0);
    WebSpuSilence();
    ResetAudioVoiceState();
    g_EngineSoundState.bank = -1;
    SetSoundSlotVoicesEnabled(1);
    SetLoadedTableVolumeScale(s_carVolumeScales[(u32)s.variant < CAR_SOUND_VOLUME_SCALE_COUNT ? s.variant : 0]);
    OpenOtherEngines(archive, race);
    s.lastTick = race->tick;
    s.lastStepTick = race->drivers[localSeat].stepTick;
    s.phase = race->phase;
    s.status = driver->status;
    s.lap = driver->car.lap;
    s.motionState = driver->car.drive.motionState;
    s.countdownCue = 0;
    s.raceCueFlags = s.raceCueDelay = s.wrongWayTimer = 0;
    s.finishFollowupCue = -1;
    s.finishFollowupWait = 0;
    s.bestLap = INT32_MAX;
    s.frame = 0;
    s.pendingFrames = 0;
    s.active = 1;
    return 1;
}

void WebAudioStopRace(void) {
    s.active = 0;
    s.otherCount = 0;
    s.pendingFrames = 0;
    WebSpuReset();
}

void WebAudioEnable(int enabled) {
    s.enabled = enabled != 0;
    s.pendingFrames = 0;
}

const int16_t *WebAudioPending(int *frames) {
    *frames = s.pendingFrames;
    s.pendingFrames = 0;
    return s.pending;
}

int WebAudioEngineRpm(void) { return s.active ? s.engine.rpm + s.engine.jitter : 0; }

/* ---- the race scene's sound calls --------------------------------------------- */

enum {
    COUNTDOWN_FIRST_CUE = 0x1E, /* race/countdown_cues.c: 3, 2, 1, GO */
    BEST_LAP_SOUND_CUE = 0x26,  /* race/lap_and_finish.c */
    BEST_LAP_CUE_DELAY = 0x96,
    TWO_LAPS_OUT_SOUND_CUE = 0x27,
    ONE_LAP_OUT_SOUND_CUE = 0x28,
    LAST_LAP_SOUND_CUE = 0x29,
    FINISH_SOUND_CUE = 0x2A,    /* track/trigger_race_cues.c */
    FINISH_FOLLOWUP_SOUND_CUE = 0x2B,
    SPEED_SOUND_CUE = 0x23,
    WRONG_WAY_SOUND_CUE = 0x2C, /* race/race_scene.c */
    LAP_CUE_FLAG_MASK = 0xF,
    LAP_CUE_ARM_DELAY = 2,
    FINISH_CUE_FLAG = 8,
    FIRST_SPEED_CUE_FLAG = 0x10,
    FINISH_CUE_SPECIAL_VOICE_GROUP = 4,
    FINISH_FOLLOWUP_MAX_WAIT_FRAMES = 60,
    WRONG_WAY_COUNTER_RESET = 81,
    WRONG_WAY_FINISH_SOUND_LIMIT = 10,
    COLLISION_SOUND_TIMER_LIMIT = 0xB, /* car/player_car_collision.c */
    COLLISION_SOUND_CLOSE_LATERAL_DISTANCE = 30,
    AMBIENCE_MAX_VOLUME = 0x60,  /* track/update_zone_ambience.c */
    AMBIENCE_FADE_DISTANCE = 800,
    POINT_AMBIENCE_MAX_LEVEL = 0x30, /* track/update_point_ambience.c */
    POINT_AMBIENCE_VOLUME_BIAS = 0x20,
    EVENT_SOUND_SPEED_SCALE = 12775, /* track/update_track_event_sound.c */
    LATERAL_LEAN_DEAD_ZONE = 0x100,
    ENGINE_NOTE = 0x3C,
};

static void UpdateFinishFollowupCue(void) {
    if (s.finishFollowupCue < 0) return;
    if (SpuGetKeyStatus((u_long)g_SpecialVoiceBits[FINISH_CUE_SPECIAL_VOICE_GROUP]) != 0 &&
        s.finishFollowupWait < FINISH_FOLLOWUP_MAX_WAIT_FRAMES) {
        s.finishFollowupWait++;
        return;
    }
    PlaySoundCue(s.finishFollowupCue);
    s.finishFollowupCue = -1;
}

/* The countdown's announcer lines, on the seconds the HUD shows. */
static void PlayCountdownCues(const RaceSim *race) {
    s32 due;
    if (race->phase == SIM_COUNTDOWN) {
        const s32 second = (s32)((race->countdown + SIM_TICK_RATE - 1) / SIM_TICK_RATE);
        due = second >= 3 ? 1 : 4 - second; /* 3, 2, 1 */
    } else {
        due = 4; /* GO */
    }
    while (s.countdownCue < due) PlaySoundCue(COUNTDOWN_FIRST_CUE + s.countdownCue++);
}

/* lap_and_finish.c CrossTheLine/RecordBestLap and CountDownTheLaps. */
static void UpdateLapCues(const RaceSim *race, const SimDriver *driver) {
    const s32 lap = driver->car.lap;
    if (lap > s.lap && lap > 1) {
        const s32 time = RaceLapTime(race, s.seat, lap - 2);
        s.raceCueFlags &= LAP_CUE_FLAG_MASK;
        if (s.raceCueDelay == 0) s.raceCueDelay = LAP_CUE_ARM_DELAY;
        if (time > 0 && time < s.bestLap) {
            s.bestLap = time;
            if (race->laps >= lap) {
                PlaySoundCue(BEST_LAP_SOUND_CUE);
                s.raceCueDelay = BEST_LAP_CUE_DELAY;
            }
        }
    } else if (lap > s.lap) {
        s.raceCueFlags &= LAP_CUE_FLAG_MASK;
        if (s.raceCueDelay == 0) s.raceCueDelay = LAP_CUE_ARM_DELAY;
    }
    s.lap = lap;
    if (s.raceCueDelay == LAP_CUE_ARM_DELAY) {
        switch (race->laps - lap) {
        case 2: PlaySoundCue(TWO_LAPS_OUT_SOUND_CUE); break;
        case 1: PlaySoundCue(ONE_LAP_OUT_SOUND_CUE); break;
        case 0: PlaySoundCue(LAST_LAP_SOUND_CUE); break;
        default: break;
        }
        s.raceCueDelay--;
    } else if (s.raceCueDelay == 1) {
        s.raceCueDelay = 0;
    } else if (s.raceCueDelay > 0) {
        s.raceCueDelay--;
    }
}

/* car/player_car_collision.c PlayPlayerCollisionSound. The simulation keeps
 * only that the car hit another; the side comes from the nearest car. */
static void PlayCollisionCue(const RaceSim *race, const SimDriver *driver) {
    const PlayerCarRuntime *car = &driver->car;
    s32 best = INT32_MAX, lateral = 0, ahead = 0;
    if (!driver->crashed || (s16)car->motionTimer >= COLLISION_SOUND_TIMER_LIMIT) return;
    for (int seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) {
        const SimDriver *other = &race->drivers[seat];
        int64_t dx, dz, distance;
        if (seat == s.seat || other->status != SIM_DRIVING || other->car.activeFlag == -1) continue;
        dx = (int64_t)other->car.x - car->x;
        dz = (int64_t)other->car.z - car->z;
        distance = dx * dx + dz * dz;
        if (distance < (int64_t)best * best || best == INT32_MAX) {
            best = (s32)sqrt((double)distance);
            lateral = other->car.trackLateralOffset - car->trackLateralOffset;
            ahead = (s32)(((int64_t)other->car.trackProgress - car->trackProgress + race->route.length +
                           race->route.length / 2) % race->route.length) - race->route.length / 2;
        }
    }
    if (best == INT32_MAX) return;
    if (abs(lateral) < COLLISION_SOUND_CLOSE_LATERAL_DISTANCE) PlaySoundCue(ahead >= 0 ? 0xA : 0xD);
    else PlaySoundCue(lateral < 0 ? 0xB : 0xC);
}

/* car/update_car_drivetrain.c PlayMotionVoice. */
static void PlayMotionVoice(const SimDriver *driver) {
    const PlayerCarRuntime *car = &driver->car;
    switch (car->drive.motionState) {
    case CAR_MOTION_DRIVING: PlayCarDrivingVoice(car, &driver->spec); break;
    case CAR_MOTION_TAKEOFF: PlayCarLaunchVoice(car); break;
    case CAR_MOTION_AIRBORNE: PlayCarAirborneVoice(car); break;
    case CAR_MOTION_STANDING_START: PlayCarStandingStartVoice(car); break;
    default: break;
    }
}

static void SimZoneAmbience(const RaceSim *race, s32 position) {
    const s32 tier = s.classIndex % GRAND_PRIX_FINAL_CLASS_INDEX;
    const s32 maximum = tier >= 1 ? AMBIENCE_MAX_VOLUME : 0;
    s32 volume = 0;
    position = TrackPositionForSeries(position, race->route.length, race->reverse);
    for (int i = 0; race->events && i < TRACK_AMBIENCE_ZONE_COUNT; ++i) {
        const TrackAmbienceZone *zone = &race->events->ambienceZones[i];
        if (zone->start == -1) break;
        if (position < zone->start || position > zone->end) continue;
        if ((int64_t)position < (int64_t)zone->start + AMBIENCE_FADE_DISTANCE && (zone->flags & 1))
            volume = (s32)((int64_t)maximum * ((int64_t)position - zone->start) / AMBIENCE_FADE_DISTANCE);
        else if ((int64_t)position > (int64_t)zone->end - AMBIENCE_FADE_DISTANCE && (zone->flags & 2))
            volume = (s32)((int64_t)maximum * ((int64_t)zone->end - position) / AMBIENCE_FADE_DISTANCE);
        else
            volume = maximum;
        break;
    }
    SetStereoSoundCue(tier >= 3 ? 1 : 0, volume, volume);
}

/* track/update_point_ambience.c with the in-car camera (camera_common.c:
 * the car's position and body yaw). */
static void SimPointAmbience(const RaceSim *race, const PlayerCarRuntime *car) {
    const s32 position = TrackPositionForSeries(car->trackProgress, race->route.length, race->reverse);
    const TrackPointAmbienceZone *zone = NULL;
    s32 level = 0, left = 0, right = 0, cue;
    for (int i = 0; race->events && i < TRACK_POINT_AMBIENCE_ZONE_COUNT; ++i) {
        const TrackPointAmbienceZone *candidate = &race->events->pointAmbienceZones[i];
        if (candidate->start == -1) break;
        if (position >= candidate->start && position <= candidate->end) {
            zone = candidate;
            break;
        }
    }
    if (zone) {
        const s32 fadeIn = (s16)zone->fadeInDistance, fadeOut = (s16)zone->fadeOutDistance;
        if (fadeIn > 0 && (int64_t)position < (int64_t)zone->start + fadeIn)
            level = (s32)(((int64_t)position - zone->start) * POINT_AMBIENCE_MAX_LEVEL / fadeIn);
        else if (fadeOut > 0 && (int64_t)position > (int64_t)zone->end - fadeOut)
            level = (s32)(((int64_t)zone->end - position) * POINT_AMBIENCE_MAX_LEVEL / fadeOut);
        else
            level = POINT_AMBIENCE_MAX_LEVEL;
    }
    if (level != 0) {
        const int64_t dx = (int64_t)zone->sourceX - car->x, dz = (int64_t)zone->sourceZ - car->z;
        const uint64_t x = (uint64_t)(dx < 0 ? -dx : dx), z = (uint64_t)(dz < 0 ? -dz : dz);
        const uint64_t radius = (uint64_t)level * 64;
        s32 attenuated = 0, sine = 0;
        if (x < radius && z < radius && x * x + z * z < radius * radius) {
            const s32 distance = (s32)sqrt((double)(x * x + z * z));
            attenuated = distance < level ? level - distance : 0;
        }
        if (attenuated != 0)
            sine = SinAngle((s32)(((u32)car->bodyYaw - 0xC00u + (u32)Atan2((s32)dx, (s32)dz)) & 0xFFFu));
        left = level + (attenuated * sine) / 4096 + POINT_AMBIENCE_VOLUME_BIAS;
        right = level + (-attenuated * sine) / 4096 + POINT_AMBIENCE_VOLUME_BIAS;
    }
    cue = level != 0 && zone && (zone->cue == 1 || zone->cue == -1) ? 2 : 3;
    SetStereoSoundCue(cue, (s16)right, (s16)left);
}

/* track/update_track_event_sound.c: the rumble of a wall on the side the car
 * leans towards. */
static void SimTrackEventSound(const RaceSim *race, const PlayerCarRuntime *car) {
    s32 flags = 0, lean, left = 0, right = 0;
    for (int i = 0; race->events && i < TRACK_EVENT_SOUND_ZONE_COUNT; ++i) {
        const TrackEventSoundZone *zone = &race->events->eventSoundZones[i];
        if (zone->start == -1) break;
        if (car->trackSection >= zone->start && car->trackSection <= zone->end) {
            flags = zone->flags;
            break;
        }
    }
    lean = car->normalizedLateralOffset;
    lean = lean < 0 ? (lean + LATERAL_LEAN_DEAD_ZONE > 0 ? 0 : lean + LATERAL_LEAN_DEAD_ZONE)
                    : (lean - LATERAL_LEAN_DEAD_ZONE < 0 ? 0 : lean - LATERAL_LEAN_DEAD_ZONE);
    if (flags != 0 && lean != 0 && race->route.count > 0) {
        const s32 point = car->trackPointIndex;
        const s32 angle = (s32)(((u32)car->bodyYaw - ANGLE_THREE_QUARTER_TURN +
                                 (u32)race->route.points[(u32)point % race->route.count].angle) & ANGLE_MASK);
        s32 cosine;
        lean = (s32)((int64_t)lean * car->speed / EVENT_SOUND_SPEED_SCALE);
        cosine = (s32)((int64_t)lean * CosAngle(angle) / 4096);
        if (lean < 0 && (flags & 2)) {
            left = -lean - cosine;
            right = -lean + cosine;
        } else if (lean > 0 && (flags & 1)) {
            right = lean + cosine;
            left = lean - cosine;
        }
    }
    SetPanVoiceTargetVolume(left, right);
}

/* track/trigger_race_cues.c. */
static void SimRaceCues(const RaceSim *race, const PlayerCarRuntime *car) {
    const int series = race->reverse ? 1 : 0;
    const TrackFinishCue *finish;
    if (!race->events) return;
    finish = &race->events->raceCues.finish[series];
    if (!(s.raceCueFlags & FINISH_CUE_FLAG) && car->trackSection == finish->trackSection && car->lap == race->laps) {
        s.raceCueFlags |= FINISH_CUE_FLAG;
        if (s.wrongWayTimer < WRONG_WAY_FINISH_SOUND_LIMIT) PlaySoundCue(FINISH_SOUND_CUE);
    }
    if (s.wrongWayTimer != 0) return;
    for (int index = 0; index < TRACK_SPEED_CUE_COUNT; ++index) {
        const TrackSpeedCue *cue = &race->events->raceCues.speed[series][index];
        const s32 flag = FIRST_SPEED_CUE_FLAG << index;
        if (s.raceCueFlags & flag) continue;
        if (cue->trackSection == -1) return;
        if (car->trackSection != cue->trackSection) continue;
        if (car->speed > (s32)((int64_t)cue->speedPercent * car->drive.speedScale) / 100 && car->motionMode <= 0) {
            s.raceCueFlags |= flag;
            PlaySoundCue(SPEED_SOUND_CUE);
        }
        return;
    }
}

static void UpdateWrongWay(const RaceSim *race, const PlayerCarRuntime *car) {
    if (car->facingBackwards == race->reverse) {
        s.wrongWayTimer = 0;
        return;
    }
    s.wrongWayTimer = s.wrongWayTimer >= WRONG_WAY_COUNTER_RESET - 1 ? WRONG_WAY_WARNING_FRAMES : s.wrongWayTimer + 1;
    if (s.wrongWayTimer >= WRONG_WAY_WARNING_FRAMES && (u8)s.frame == 0) PlaySoundCue(WRONG_WAY_SOUND_CUE);
}

static void UpdateOtherEngines(const RaceSim *race) {
    const PlayerCarRuntime *listener = &race->drivers[s.seat].car;
    PlayerEngineTable player;
    SaveEngineTable(&player);
    for (int k = 0; k < s.otherCount; ++k) {
        OtherEngine *other = &s.others[k];
        const SimDriver *driver = &race->drivers[other->seat];
        const int firstVoice = OTHER_ENGINE_FIRST_VOICE + k * OTHER_ENGINE_LAYERS;
        const int audible = driver->status == SIM_DRIVING && driver->car.activeFlag != -1 &&
                            (race->phase == SIM_COUNTDOWN || race->phase == SIM_RACING);
        double dx, dz, distance, gain;
        s32 position, bank, pan;
        if (!audible) {
            if (other->keyed)
                for (int layer = 0; layer < OTHER_ENGINE_LAYERS; ++layer) SsUtKeyOffV(firstVoice + layer);
            other->keyed = 0;
            continue;
        }
        StepEngineSound(&other->sound, &driver->car.drive, &driver->spec, s.frame, driver->random);
        memcpy(g_EngineSoundCurves, other->curves, sizeof(other->curves));
        g_EngineSoundState.maxRpm = other->maxRpm;
        bank = other->sound.powered ? 1 : 0;
        position = other->maxRpm > 0 ? (s32)((int64_t)(other->sound.rpm + other->sound.jitter) * 10240 / other->maxRpm) : 0;
        dx = (double)driver->car.x - listener->x;
        dz = (double)driver->car.z - listener->z;
        distance = sqrt(dx * dx + dz * dz);
        /* Heard from outside the car: at most half the local engine's level,
         * falling off with distance beyond a few car lengths. */
        gain = 0.5 * (distance <= 3000.0 ? 1.0 : 3000.0 / distance);
        pan = SinAngle((s32)(((u32)listener->bodyYaw - 0xC00u + (u32)Atan2((s32)dx, (s32)dz)) & 0xFFFu));
        for (int layer = 0; layer < OTHER_ENGINE_LAYERS; ++layer) {
            const int voice = firstVoice + layer;
            const s16 program = other->tones[layer][bank];
            s32 bend, volume, left, right;
            if (!other->keyed || (bank != other->bank && other->tones[layer][0] != other->tones[layer][1]))
                SsUtKeyOnV(voice, other->vab, program, 0, ENGINE_NOTE, 0, 0, 0);
            bend = InterpolateAudioParameter(layer * 2, position, bank);
            volume = ScaleClampedVoiceVolume(InterpolateAudioParameter(layer * 2 + 1, position, bank), other->volumeScale);
            volume = (s32)(ScaleClampedVoiceVolume(volume, g_SoundScale.scale) * gain);
            left = volume - volume * pan / 8192;
            right = volume + volume * pan / 8192;
            SsUtSetVVol(voice, (s16)left, (s16)right);
            SsUtPitchBend(voice, other->vab, program, ENGINE_NOTE, bend);
        }
        other->keyed = 1;
        other->bank = bank;
    }
    RestoreEngineTable(&player);
}

/* One retail game frame (25 Hz) of race audio. */
static void AudioFrame(const RaceSim *race) {
    const SimDriver *driver = &race->drivers[s.seat];
    const PlayerCarRuntime *car = &driver->car;
    const int stepped = driver->stepTick != s.lastStepTick && driver->stepTick == race->tick;
    const int finishedNow = driver->status != SIM_DRIVING && s.status == SIM_DRIVING;
    s.frame++;
    UpdateFinishFollowupCue();
    PlayCountdownCues(race);
    if (finishedNow) {
        /* lap_and_finish.c FinishRace / race retire: the car's sounds stop and
         * only the announcer carries on. */
        StopEffects();
        SetPanVoiceTargetVolume(0, 0);
        for (int cue = 0; cue < AUDIO_SOUND_MODE_COUNT; ++cue) SetStereoSoundCue(cue, 0, 0);
        if (driver->status == SIM_DRIVER_FINISHED) {
            s.raceCueFlags |= FINISH_CUE_FLAG;
            s.finishFollowupCue = FINISH_FOLLOWUP_SOUND_CUE;
            s.finishFollowupWait = 0;
        }
    }
    s.status = driver->status;
    if (driver->status != SIM_DRIVING) {
        UpdateOtherEngines(race);
        UpdateBasicEffectVoices();
        return;
    }
    UpdateLapCues(race, driver);
    /* update_player_car.c: drivetrain voice, contact cues, engine note. */
    if (stepped) {
        const DriverStep *step = &driver->step;
        const int audible = race->phase == SIM_RACING;
        PlayMotionVoice(driver);
        if (s.motionState == CAR_MOTION_DRIVING && car->drive.motionState == CAR_MOTION_TAKEOFF)
            SetIndexedEffectVoice(0, 0, 0);
        if (step->motionFinished) SetIndexedEffectVoice(-1, 0, 0);
        if (audible) PlayCollisionCue(race, driver);
        PlayPlayerLandingCue(step->landingFrames, audible);
        PlayPlayerContactCue(car, step->skid, step->skidAngle, audible);
        s.lastStepTick = driver->stepTick;
    }
    s.motionState = car->drive.motionState;
    StepEngineSound(&s.engine, &car->drive, &driver->spec, s.frame, driver->random);
    /* race_scene.c after the player car: wrong way, ambience, race cues. */
    UpdateWrongWay(race, car);
    SimZoneAmbience(race, car->trackProgress);
    SimPointAmbience(race, car);
    SimTrackEventSound(race, car);
    SimRaceCues(race, car);
    UpdateLoadedAudioVoices(WrapSigned32((int64_t)s.engine.rpm + s.engine.jitter), s.engine.powered);
    UpdateOtherEngines(race);
}

void WebAudioTick(const RaceSim *race) {
    if (!s.active || !race) return;
    if (race->tick != s.lastTick) {
        /* The retail game frame is every second 50 Hz tick, as the physics. */
        if ((race->phase == SIM_COUNTDOWN || race->phase == SIM_RACING || race->phase == SIM_FINISHED) &&
            race->tick / SIM_PHYSICS_INTERVAL != s.lastTick / SIM_PHYSICS_INTERVAL)
            AudioFrame(race);
        s.lastTick = race->tick;
        s.phase = race->phase;
    }
    if (!s.enabled) return;
    if (s.pendingFrames + WEB_AUDIO_FRAMES_PER_TICK > PENDING_CAPACITY) s.pendingFrames = 0;
    WebSpuRender(&s.pending[s.pendingFrames * 2], WEB_AUDIO_FRAMES_PER_TICK);
    s.pendingFrames += WEB_AUDIO_FRAMES_PER_TICK;
}
