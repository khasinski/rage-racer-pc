#include <psyz/audio.h>
#include <psyz/spu.h>

#include <stdint.h>
#include <stdio.h>

static void Write16(FILE *file, uint16_t value) {
    fputc(value & 0xff, file);
    fputc(value >> 8, file);
}

static void Write32(FILE *file, uint32_t value) {
    Write16(file, (uint16_t)value);
    Write16(file, (uint16_t)(value >> 16));
}

static int WriteFixture(const char *path) {
    static const int16_t samples[] = {1000, -1000, 2000, -2000};
    FILE *file = fopen(path, "wb");
    if (!file) return 0;
    fwrite("RIFF", 1, 4, file); Write32(file, 36 + sizeof(samples));
    fwrite("WAVEfmt ", 1, 8, file); Write32(file, 16); Write16(file, 1);
    Write16(file, 2); Write32(file, 44100); Write32(file, 176400);
    Write16(file, 4); Write16(file, 16); fwrite("data", 1, 4, file);
    Write32(file, sizeof(samples)); fwrite(samples, 1, sizeof(samples), file);
    return fclose(file) == 0;
}

#define CHECK(condition) do { if (!(condition)) {                            \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); return 1;       \
} } while (0)

int main(void) {
    const char *path = "pcm_music_test.wav";
    short output[8] = {0};
    CHECK(WriteFixture(path));
    CHECK(Psyz_PcmMusicLoad(path) == 0);
    CHECK(Psyz_PcmMusicIsLoaded());
    Psyz_SpuInit();
    Psyz_PcmMusicSetVolume(127);
    Psyz_PcmMusicPlay(1);
    Psyz_SpuPullSamples(output, 3);
    CHECK(output[0] == 1000 && output[1] == -1000);
    CHECK(output[2] == 2000 && output[3] == -2000);
    CHECK(output[4] == 1000 && output[5] == -1000);
    Psyz_PcmMusicStop();
    output[0] = output[1] = 123;
    Psyz_SpuPullSamples(output, 1);
    CHECK(output[0] == 0 && output[1] == 0);
    Psyz_PcmMusicUnload();
    CHECK(!Psyz_PcmMusicIsLoaded());
    remove(path);
    return 0;
}
