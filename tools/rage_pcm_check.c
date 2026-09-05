#include "pcm_metrics.h"
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>

static int ParseCount(const char *text, uint64_t *value) {
    char *end;
    if (*text < '0' || *text > '9') return 0;
    errno = 0;
    unsigned long long parsed = strtoull(text, &end, 10);
    if (errno != 0 || *end != '\0' || parsed > UINT64_MAX) return 0;
    *value = (uint64_t)parsed;
    return 1;
}

int main(int argc, char **argv) {
    RagePcmMetrics expected, measured;
    if (argc != 4 || !ParseCount(argv[2], &expected.frames) ||
        !ParseCount(argv[3], &expected.absoluteEnergy)) {
        fprintf(stderr, "usage: rage-pcm-check PCM_S16LE FRAMES ABSOLUTE_ENERGY\n");
        return 2;
    }
    FILE *file = fopen(argv[1], "rb");
    if (file == NULL) { perror(argv[1]); return 1; }
    int ok = Pcm16StereoMeasure(file, &measured);
    if (fclose(file) != 0) ok = 0;
    if (!ok) { fprintf(stderr, "invalid or unreadable stereo PCM\n"); return 1; }
    printf("pcm frames=%" PRIu64 " energy=%" PRIu64 "\n",
           measured.frames, measured.absoluteEnergy);
    if (measured.frames == 0 || measured.absoluteEnergy == 0 ||
        measured.frames != expected.frames ||
        measured.absoluteEnergy != expected.absoluteEnergy) {
        fprintf(stderr, "silent PCM or mismatch against reported mixer metrics\n");
        return 1;
    }
    return 0;
}
