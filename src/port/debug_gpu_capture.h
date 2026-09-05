#ifndef RAGE_DEBUG_GPU_CAPTURE_H
#define RAGE_DEBUG_GPU_CAPTURE_H
#include <stdint.h>
void DebugGpuCaptureInit(void);
void DebugGpuCaptureRequest(uint64_t sourceFrame);
void DebugGpuCapturePoll(void);
#endif
