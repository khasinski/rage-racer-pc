#ifndef RAGE_PCM_METRICS_H
#define RAGE_PCM_METRICS_H
#include <stdint.h>
#include <stdio.h>
typedef struct RagePcmMetrics {
    uint64_t frames;
    uint64_t absoluteEnergy;
} RagePcmMetrics;
/* Count signed 16-bit little-endian stereo PCM from the current file offset.
 * Reject incomplete stereo frames/read errors/overflow; clear output on error. */
int Pcm16StereoMeasure(FILE *file, RagePcmMetrics *out);
#endif
