/* The browser's sound chip: the libsnd utility calls the retail race audio
 * makes (SsUtKeyOnV, SsUtPitchBend, SsUtSetVVol, ...) over a software model of
 * the PlayStation SPU voices, as the desktop's PsyZ backend runs them
 * (external/psyz/psyz/src/psyz/psyz_spu.c, libsnd.c and the decompiled
 * libsnd note/volume maths). VAB banks are decoded straight from the disc's
 * sample bodies; the output is 44.1 kHz interleaved stereo. */
#ifndef WEB_SPU_H
#define WEB_SPU_H

#include <stddef.h>
#include <stdint.h>

enum {
    WEB_SPU_RATE = 44100,
    /* The SPU's 24 hardware voices, then voices the browser adds for the
     * engines of the other human cars (web_audio.c). */
    WEB_SPU_HARDWARE_VOICES = 24,
    WEB_SPU_VOICES = 40,
    WEB_SPU_VAB_SLOTS = 8,
};

/* Opens a VAB (header + sample body, both copied) under the fixed libsnd id
 * vabId. Returns vabId, or -1 for malformed data. */
int WebSpuOpenVab(int vabId, const uint8_t *header, size_t headerSize,
                  const uint8_t *body, size_t bodySize);
void WebSpuCloseVab(int vabId);
/* Keys every voice off at once, without a release. */
void WebSpuSilence(void);
void WebSpuReset(void);
/* Mixes frames of interleaved stereo 16-bit output. */
void WebSpuRender(int16_t *out, int frames);

#endif
