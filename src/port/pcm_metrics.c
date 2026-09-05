#include "pcm_metrics.h"

int Pcm16StereoMeasure(FILE *file, RagePcmMetrics *out) {
    unsigned char bytes[4096];
    RagePcmMetrics result = {0};
    if (out == NULL) return 0;
    *out = result;
    if (file == NULL) return 0;
    for (;;) {
        size_t count = fread(bytes, 1, sizeof(bytes), file);
        if (ferror(file) || count % 4 != 0 ||
            result.frames > UINT64_MAX - count / 4) return 0;
        result.frames += count / 4;
        for (size_t i = 0; i < count; i += 2) {
            int sample = bytes[i] | ((unsigned)bytes[i + 1] << 8);
            if (sample >= 32768) sample -= 65536;
            uint64_t magnitude = (uint64_t)(sample < 0 ? -sample : sample);
            if (result.absoluteEnergy > UINT64_MAX - magnitude) return 0;
            result.absoluteEnergy += magnitude;
        }
        if (count != sizeof(bytes)) break;
    }
    *out = result;
    return 1;
}
