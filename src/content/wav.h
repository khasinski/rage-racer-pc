#ifndef RAGE_WAV_H
#define RAGE_WAV_H
#include <stddef.h>
#include <stdint.h>
enum { WAV_MAX_BYTES = 128 * 1024 * 1024 };
/* Canonical runtime PCM: 44.1 kHz stereo signed 16-bit little endian. */
int WavInfo(const char *path, uint32_t *frames);
int WavWrite(const char *path, const void *samples, size_t bytes);
#endif
