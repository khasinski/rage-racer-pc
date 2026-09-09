#ifndef RAGE_DISC_STAGE_VALIDATION_H
#define RAGE_DISC_STAGE_VALIDATION_H

#include <stddef.h>
#include <stdint.h>

enum { RAGE_DISC_STAGE_SHA1_BYTES = 20 };

void RageDiscStageSha1(const void *data, size_t size,
                       uint8_t digest[RAGE_DISC_STAGE_SHA1_BYTES]);
void RageDiscStageSha1Hex(const uint8_t digest[RAGE_DISC_STAGE_SHA1_BYTES],
                          char hex[RAGE_DISC_STAGE_SHA1_BYTES * 2 + 1]);
int RageDiscStageValidatePsxExe(const void *data, size_t size,
                                const char *expectedSha1);

#endif
