#include "pcm_metrics.h"
#include <stdio.h>
#include <string.h>
static int failures;
#define CHECK(x) do { if (!(x)) { ++failures; \
    fprintf(stderr, "line %d: %s\n", __LINE__, #x); } } while (0)
int main(void) {
    const unsigned char frame[] = {0, 128, 255, 127}; /* -32768, +32767 */
    RagePcmMetrics metrics = {99, 99};
    FILE *file = tmpfile();
    CHECK(file != NULL);
    if (file == NULL) return 1;
    CHECK(Pcm16StereoMeasure(file, &metrics));
    CHECK(metrics.frames == 0 && metrics.absoluteEnergy == 0);
    for (unsigned i = 0; i < 2050; ++i) CHECK(fwrite(frame, 1, 4, file) == 4);
    rewind(file);
    CHECK(Pcm16StereoMeasure(file, &metrics));
    CHECK(metrics.frames == 2050 && metrics.absoluteEnergy == UINT64_C(2050) * 65535);
    CHECK(fseek(file, 0, SEEK_END) == 0);
    CHECK(fwrite(frame, 1, 3, file) == 3);
    rewind(file);
    CHECK(!Pcm16StereoMeasure(file, &metrics));
    CHECK(metrics.frames == 0 && metrics.absoluteEnergy == 0);
    CHECK(!Pcm16StereoMeasure(NULL, &metrics));
    CHECK(!Pcm16StereoMeasure(file, NULL));
    CHECK(fclose(file) == 0);
    return failures != 0;
}
