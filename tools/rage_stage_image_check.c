/* Image invariants for the compiled stage-regression runner. No game data. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { WIDTH = 240, HEIGHT = 180, BYTES = WIDTH * HEIGHT * 3, STEPS = 24 };
typedef struct Shape { int area, left, right, top, bottom; } Shape;

static Shape Measure(const unsigned char *pixels) {
    Shape s = {0, WIDTH, -1, HEIGHT, -1};
    for (int y = 0; y < HEIGHT; ++y) for (int x = 0; x < WIDTH; ++x) {
        const unsigned char *p = pixels + (y * WIDTH + x) * 3;
        if (!(p[0] || p[1] || p[2])) continue;
        ++s.area;
        if (x < s.left) s.left = x;
        if (x > s.right) s.right = x;
        if (y < s.top) s.top = y;
        if (y > s.bottom) s.bottom = y;
    }
    return s;
}
static int Border(Shape s) {
    return s.left <= 0 || s.top <= 0 || s.right >= WIDTH - 1 || s.bottom >= HEIGHT - 1;
}
static int Track(const unsigned char *pixels) {
    Shape s = Measure(pixels);
    return s.area >= WIDTH * HEIGHT / 200 && !Border(s);
}
static int ReadStream(FILE *f, unsigned char *pixels) {
    char header[64];
    const char expected[] = "P6\n240 180\n255\n";
    if (!f) return 0;
    /* The stage emits this exact header. Reject wrong sizes and short/trailing data. */
    return fread(header, 1, sizeof(expected) - 1, f) == sizeof(expected) - 1 &&
        memcmp(header, expected, sizeof(expected) - 1) == 0 &&
        fread(pixels, 1, BYTES, f) == BYTES && fgetc(f) == EOF && !ferror(f);
}
static int Read(const char *path, unsigned char *pixels) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    int ok = ReadStream(f, pixels);
    if (fclose(f)) ok = 0;
    if (!ok) fprintf(stderr, "Invalid stage PPM: %s\n", path);
    return ok;
}
static int Rotation(const unsigned char *left, const unsigned char *right) {
    Shape a = Measure(left), b = Measure(right);
    if (a.area != b.area || a.left != b.left || a.right != b.right ||
        a.top != b.top || a.bottom != b.bottom) return 0;
    for (int i = 0; i < BYTES; ++i)
        if (abs((int)left[i] - right[i]) > 2) return 0;
    return 1;
}
static int Sweep(unsigned char *pixels) {
    Shape shapes[STEPS];
    int widest = 0, tallest = 0;
    for (int i = 0; i < STEPS; ++i) {
        Shape s = shapes[i] = Measure(pixels + i * BYTES);
        if (s.area <= WIDTH * HEIGHT / 100 || s.area >= WIDTH * HEIGHT * 6 / 10 || Border(s)) return 0;
        if (s.right - s.left + 1 > widest) widest = s.right - s.left + 1;
        if (s.bottom - s.top + 1 > tallest) tallest = s.bottom - s.top + 1;
    }
    if (widest < WIDTH / 4 || tallest < HEIGHT / 5) return 0;
    for (int i = 0; i < STEPS; ++i) {
        int next = (i + 1) % STEPS;
        if (!memcmp(pixels + i * BYTES, pixels + next * BYTES, BYTES)) return 0;
        if (shapes[next].area * 10 < shapes[i].area * 6 ||
            shapes[next].area * 10 > shapes[i].area * 17) return 0;
    }
    for (int i = 1; i < STEPS / 2; ++i) {
        int a = shapes[i].area, b = shapes[STEPS - i].area;
        if (abs(a - b) * 100 > (a > b ? a : b) * 8) return 0;
    }
    return 1;
}
int main(int argc, char **argv) {
    int count, ok = 1;
    if (argc < 3) return 2;
    int sweep = !strcmp(argv[1], "sweep");
    int equal = !strcmp(argv[1], "equal");
    int rotation = !strcmp(argv[1], "rotation");
    int track = !strcmp(argv[1], "track");
    count = argc - 2;
    if ((!sweep && !equal && !rotation && !track) ||
        (sweep && count != STEPS) || ((equal || rotation) && count != 2) || (track && count != 1)) return 2;
    unsigned char *pixels = malloc((size_t)count * BYTES);
    if (!pixels) return 2;
    for (int i = 0; i < count; ++i) if (!Read(argv[i + 2], pixels + i * BYTES)) { ok = 0; break; }
    if (ok && sweep) ok = Sweep(pixels);
    if (ok && equal) ok = !memcmp(pixels, pixels + BYTES, BYTES);
    if (ok && rotation) ok = Rotation(pixels, pixels + BYTES);
    if (ok && track) ok = Track(pixels);
    free(pixels);
    if (!ok) fprintf(stderr, "Stage image invariant failed: %s\n", argv[1]);
    return ok ? 0 : 1;
}
