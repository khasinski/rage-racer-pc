#define main StageImageCommand
#include "../../tools/rage_stage_image_check.c"
#undef main

static void FixtureSize(unsigned char *p, int width, int height) {
    memset(p, 0, STEPS * BYTES);
    for (int i = 0; i < STEPS; ++i)
        for (int y = 10; y < 10 + height; ++y)
            for (int x = 10; x < 10 + width; ++x)
                p[i * BYTES + (y * WIDTH + x) * 3] = (unsigned char)(i + 1);
}
static void Fixture(unsigned char *p) { FixtureSize(p, 80, 40); }
static int ParserCase(const char *header, size_t bytes, int extra, int expected) {
    FILE *f = tmpfile();
    unsigned char pixels[BYTES] = {0};
    if (!f) return 0;
    int ok = fwrite(header, 1, strlen(header), f) == strlen(header) &&
        fwrite(pixels, 1, bytes, f) == bytes;
    if (extra && fputc(0, f) == EOF) ok = 0;
    rewind(f);
    ok = ok && ReadStream(f, pixels) == expected;
    if (fclose(f)) ok = 0;
    return ok;
}
int main(void) {
    if (!ParserCase("P6\n240 180\n255\n", BYTES, 0, 1) ||
        !ParserCase("", 0, 0, 0) ||
        !ParserCase("P6\n240 180\n255\n", BYTES - 1, 0, 0) ||
        !ParserCase("P6\n240 180\n255\n", BYTES, 1, 0) ||
        !ParserCase("P3\n240 180\n255\n", BYTES, 0, 0) ||
        !ParserCase("P6\n241 180\n255\n", BYTES, 0, 0) ||
        !ParserCase("P6\n240 180\n256\n", BYTES, 0, 0)) return 7;
    unsigned char *p = malloc(STEPS * BYTES);
    if (!p) return 1;
    Fixture(p);
    if (!Sweep(p)) return 2;
    memset(p + BYTES, 0, BYTES);
    if (Sweep(p)) return 3; /* Disappearing geometry. */
    Fixture(p);
    memcpy(p + BYTES, p, BYTES);
    if (Sweep(p)) return 4; /* Frozen adjacent frames. */
    Fixture(p);
    p[0] = 1;
    if (Sweep(p)) return 5; /* Clipped silhouette. */
    Fixture(p);
    for (int y = 10; y < 50; ++y)
        for (int x = 10; x < 30; ++x)
            p[BYTES + (y * WIDTH + x) * 3] = 0;
    if (Sweep(p)) return 6; /* Left/right asymmetry, without abrupt area jump. */
    Fixture(p);
    memcpy(p + BYTES, p, BYTES);
    if (!Rotation(p, p + BYTES)) return 8;
    const int sample = (10 * WIDTH + 10) * 3;
    p[BYTES + sample] += 2;
    if (!Rotation(p, p + BYTES)) return 9;
    p[BYTES + sample] += 1;
    if (Rotation(p, p + BYTES)) return 10;
    memcpy(p + BYTES, p, BYTES);
    p[BYTES + sample] = 0;
    if (Rotation(p, p + BYTES)) return 11;
    memset(p, 0, BYTES);
    if (Track(p)) return 12;
    for (int i = 0; i < WIDTH * HEIGHT / 200; ++i)
        p[((i / 20 + 10) * WIDTH + i % 20 + 10) * 3] = 1;
    if (!Track(p)) return 13; /* Exactly the minimum area. */
    p[(10 * WIDTH + 10) * 3] = 0;
    if (Track(p)) return 14;
    Fixture(p);
    p[0] = 1;
    if (Track(p)) return 15;
    FixtureSize(p, 18, 24);
    if (Sweep(p)) return 16; /* Exact excluded minimum car area. */
    FixtureSize(p, 216, 120);
    if (Sweep(p)) return 17; /* Exact excluded maximum car area. */
    FixtureSize(p, 59, 40);
    if (Sweep(p)) return 18; /* Squashed width despite adequate area. */
    FixtureSize(p, 80, 35);
    if (Sweep(p)) return 19; /* Squashed height. */
    Fixture(p);
    for (int y = 50; y < 90; ++y)
        for (int x = 10; x < 90; ++x)
            p[BYTES + (y * WIDTH + x) * 3] = 2;
    if (Sweep(p)) return 20; /* Abrupt area change. */
    Fixture(p);
    memcpy(p + (STEPS - 1) * BYTES, p, BYTES);
    if (Sweep(p)) return 21; /* Stuck across the last-to-first boundary. */
    free(p);
    return 0;
}
