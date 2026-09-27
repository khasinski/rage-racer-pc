/* Race audio for the browser: the retail race's sound calls (engine layers,
 * tyre and impact cues, the announcer, track ambience) driven from the race
 * simulation, mixed by web_spu.c. Music is streamed by the page itself from
 * the disc's CD audio tracks (web/src/audio.ts). */
#ifndef WEB_AUDIO_H
#define WEB_AUDIO_H

#include <stdint.h>

#include "game/race_data.h"
#include "game/race_sim.h"

enum {
    /* Output frames per 50 Hz simulation tick at 44.1 kHz. */
    WEB_AUDIO_FRAMES_PER_TICK = 882,
};

/* Loads the sample banks for a prepared race and resets every voice.
 * classIndex selects the ambience tier as the retail race does. */
int WebAudioStartRace(const RaceData *archive, const RaceSim *race, int localSeat, int classIndex);
void WebAudioStopRace(void);
/* After each simulation tick: the tick's sound calls, then (when enabled)
 * WEB_AUDIO_FRAMES_PER_TICK frames of output appended to the pending PCM. */
void WebAudioTick(const RaceSim *race);
/* Rendering is off until the page has an audio output. */
void WebAudioEnable(int enabled);
/* Pending interleaved stereo frames; taking them empties the buffer. */
const int16_t *WebAudioPending(int *frames);
/* The engine note the local car is sounding (displayed rpm plus jitter). */
int WebAudioEngineRpm(void);

#endif
