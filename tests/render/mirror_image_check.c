#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char *Read(const char *path, unsigned *width, unsigned *height) {
    FILE *file = fopen(path, "rb");
    char line[128];
    unsigned char *pixels = NULL;
    if (!file) return NULL;
    if (!fgets(line, sizeof(line), file) || strcmp(line, "P6\n")) goto done;
    char extra;
    if (!fgets(line, sizeof(line), file) || sscanf(line, "%u %u %c", width, height, &extra) != 2 ||
        !*width || !*height || *width > 8192 || *height > 8192) goto done;
    if (!fgets(line, sizeof(line), file) || strcmp(line, "255\n")) goto done;
    size_t size = (size_t)*width * *height * 3;
    pixels = malloc(size);
    if (pixels && (fread(pixels, 1, size, file) != size || fgetc(file) != EOF || ferror(file))) {
        free(pixels); pixels = NULL;
    }
done:
    if (fclose(file)) { free(pixels); pixels = NULL; }
    return pixels;
}
int main(int argc, char **argv) {
    if (argc != 3) return 2;
    unsigned cw = 0, ch = 0, mw = 0, mh = 0;
    unsigned char *classic = Read(argv[1], &cw, &ch);
    unsigned char *modern = Read(argv[2], &mw, &mh);
    int ok = classic && modern && cw == mw && ch == mh;
    unsigned scene = 0, bright = 0;
    if (ok) {
        for (unsigned y = 18; y < 54 && y < mh; ++y)
            for (unsigned x = 86; x < 234 && x < mw; ++x) {
                const unsigned char *p = modern + ((size_t)y * mw + x) * 3;
                if (p[0] > 25 || p[1] > 25 || p[2] > 25) ++scene;
                if (p[0] > 80 && p[1] > 80 && p[2] > 80) ++bright;
            }
        ok = scene >= 1200 && bright >= 8;
    }
    if (!ok) fprintf(stderr, "Mirror image invalid: scene=%u bodywork=%u\n", scene, bright);
    free(classic); free(modern);
    return ok ? 0 : 1;
}
