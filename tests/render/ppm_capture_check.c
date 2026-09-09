#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static unsigned char *ReadPPM(const char *path, unsigned *width, unsigned *height) {
    FILE *file = fopen(path, "rb");
    char magic[16], dimensions[64], maximum[16];
    unsigned char *pixels = NULL;
    size_t size;
    if (!file || !fgets(magic, sizeof(magic), file) ||
        !fgets(dimensions, sizeof(dimensions), file) ||
        !fgets(maximum, sizeof(maximum), file) || strcmp(magic, "P6\n") ||
        strcmp(maximum, "255\n") || sscanf(dimensions, "%u %u", width, height) != 2 ||
        !*width || !*height || *width > SIZE_MAX / (3u * *height)) goto fail;
    size = (size_t)*width * *height * 3u;
    pixels = malloc(size);
    if (!pixels || fread(pixels, 1, size, file) != size || fgetc(file) != EOF) goto fail;
    fclose(file);
    return pixels;
fail:
    if (file) fclose(file);
    free(pixels);
    return NULL;
}
static int Waterfall(const unsigned char *pixels, unsigned width, unsigned height) {
    unsigned bright = 0;
    if (width != 320 || height != 240) return 0;
    for (unsigned y = 60; y < 135; ++y) for (unsigned x = 35; x < 210; ++x) {
        const unsigned char *pixel = pixels + ((size_t)y * width + x) * 3u;
        bright += pixel[0] > 150 && pixel[1] > 150 && pixel[2] > 120;
    }
    if (bright < 3000) fprintf(stderr, "classic waterfall disappeared: %u pixels\n", bright);
    return bright >= 3000;
}
static int Pegase(const unsigned char *pixels, unsigned width, unsigned height) {
    unsigned left = 0, right = 0;
    if (width != 426 || height != 240) return 0;
    for (unsigned y = 181; y < 205; ++y) for (unsigned x = 191; x < 232; ++x) {
        const unsigned char *pixel = pixels + ((size_t)y * width + x) * 3u;
        unsigned *count = x < 212 ? &left : &right;
        *count += pixel[0] < 80 && pixel[1] < 80 && pixel[2] < 80;
    }
    if (left < 350 || right < 350 || abs((int)left - (int)right) > 100)
        fprintf(stderr, "Age Pegase cabin asymmetric: left=%u right=%u\n", left, right);
    return left >= 350 && right >= 350 && abs((int)left - (int)right) <= 100;
}
static int Boot(const unsigned char *pixels, unsigned width, unsigned height) {
    unsigned nonBlack = 0, cyan = 0, white = 0;
    if (width != 320 || height != 240) return 0;
    for (size_t i = 0; i < (size_t)width * height; ++i) {
        const unsigned char *p = pixels + i * 3u;
        nonBlack += p[0] || p[1] || p[2];
        cyan += p[1] > 70 && p[2] > 60 && p[0] < 20;
        white += p[0] > 220 && p[1] > 220 && p[2] > 220;
    }
    if (nonBlack < 8000 || cyan < 3000 || white < 500)
        fprintf(stderr, "copyright frame missing: non_black=%u cyan=%u white=%u\n", nonBlack, cyan, white);
    return nonBlack >= 8000 && cyan >= 3000 && white >= 500;
}
static int Trophy(const unsigned char *pixels, unsigned width, unsigned height) {
    unsigned bright = 0;
    if (width != 320 || height != 480) return 0;
    for (unsigned y = 52; y < 116; ++y) for (unsigned x = 32; x < 80; ++x) {
        const unsigned char *p = pixels + ((size_t)y * width + x) * 3u;
        bright += p[0] > 180 && p[1] > 180 && p[2] > 180;
    }
    if (bright < 500) fprintf(stderr, "Trophy View selector wrong texture page: %u\n", bright);
    return bright >= 500;
}
static int Prologue(const unsigned char *pixels, unsigned width, unsigned height) {
    unsigned nonBlack = 0, brightText = 0, colored = 0, lower = 0;
    if (width != 320 || height != 240) return 0;
    for (size_t i = 0; i < (size_t)width * height; ++i) {
        const unsigned char *p = pixels + i * 3u;
        unsigned minimum = p[0], maximum = p[0];
        if (p[1] < minimum) minimum = p[1];
        if (p[2] < minimum) minimum = p[2];
        if (p[1] > maximum) maximum = p[1];
        if (p[2] > maximum) maximum = p[2];
        nonBlack += maximum != 0;
        brightText += minimum > 180;
        colored += maximum > 40 && maximum - minimum > 20;
        lower += i >= 320u * 120u && maximum != 0;
    }
    if (nonBlack < 15000 || brightText < 1000 || colored < 6000 || lower < 8000)
        fprintf(stderr, "prologue missing text/cars/track: non_black=%u bright=%u colored=%u lower=%u\n", nonBlack, brightText, colored, lower);
    return nonBlack >= 15000 && brightText >= 1000 && colored >= 6000 && lower >= 8000;
}
static int Title(const char *firstPath, const char *secondPath) {
    unsigned firstWidth, firstHeight, secondWidth, secondHeight;
    unsigned char *first = ReadPPM(firstPath, &firstWidth, &firstHeight);
    unsigned char *second = ReadPPM(secondPath, &secondWidth, &secondHeight);
    size_t stable = 320u * 190u * 3u;
    int ok = first && second && firstWidth == 320 && firstHeight == 240 &&
        secondWidth == 320 && secondHeight == 240 && !memcmp(first, second, stable) &&
        (first[((size_t)175 * 320 + 135) * 3] >= 80 || first[((size_t)175 * 320 + 135) * 3 + 1] >= 80 || first[((size_t)175 * 320 + 135) * 3 + 2] >= 80) &&
        first[((size_t)175 * 320 + 138) * 3] <= 20 && first[((size_t)175 * 320 + 138) * 3 + 1] <= 20 && first[((size_t)175 * 320 + 138) * 3 + 2] <= 20;
    if (!ok) fprintf(stderr, "title artwork differs or logo UV is shifted\n");
    free(second); free(first);
    return ok;
}
static int SkyDigest(int count, char **paths, uint32_t expected) {
    uint32_t digest = 2166136261u;
    for (int i = 0; i < count; ++i) {
        FILE *file = fopen(paths[i], "rb");
        int value;
        if (!file) return 0;
        while ((value = fgetc(file)) != EOF) digest = (digest ^ (unsigned char)value) * 16777619u;
        fclose(file);
    }
    if (count < 23 || digest != expected)
        fprintf(stderr, "sky digest differs: frames=%d digest=%08x expected=%08x\n", count, digest, expected);
    return count >= 23 && digest == expected;
}
static int NonEmpty(const char *path, unsigned expectedWidth, unsigned expectedHeight) {
    unsigned width, height, nonBlack = 0;
    unsigned char *pixels = ReadPPM(path, &width, &height);
    if (!pixels) return 0;
    for (size_t i = 0; i < (size_t)width * height * 3u; ++i) nonBlack += pixels[i] != 0;
    free(pixels);
    if (width != expectedWidth || height != expectedHeight || nonBlack < 100)
        fprintf(stderr, "invalid/empty capture: %ux%u non_black=%u\n", width, height, nonBlack);
    return width == expectedWidth && height == expectedHeight && nonBlack >= 100;
}
static int GrandPrix(const unsigned char *pixels, unsigned width, unsigned height) {
    unsigned nearWhite = 0, road = 0, bright = 0, darkCar = 0;
    unsigned long lowerSum = 0;
    if (width != 320 || height != 240) return 0;
    for (unsigned y = 0; y < height; ++y) for (unsigned x = 0; x < width; ++x) {
        const unsigned char *p = pixels + ((size_t)y * width + x) * 3u;
        unsigned min = p[0], max = p[0], average = ((unsigned)p[0] + p[1] + p[2]) / 3u;
        if (p[1] < min) min = p[1];
        if (p[2] < min) min = p[2];
        if (p[1] > max) max = p[1];
        if (p[2] > max) max = p[2];
        bright += min > 180;
        if (y >= 120) { nearWhite += min > 220; road += average > 20 && average < 180 && max - min < 20; lowerSum += p[0] + p[1] + p[2]; }
        if (y >= 80 && y < 205 && x >= 35 && x < 240) darkCar += max < 70;
    }
    double mean = (double)lowerSum / (3.0 * 320.0 * 120.0);
    if (nearWhite > 2000 || road < 20000 || bright < 500 || mean <= 20 || mean >= 100 || darkCar < 12000)
        fprintf(stderr, "Grand Prix image invalid: white=%u road=%u bright=%u mean=%.1f car=%u\n", nearWhite, road, bright, mean, darkCar);
    return nearWhite <= 2000 && road >= 20000 && bright >= 500 && mean > 20 && mean < 100 && darkCar >= 12000;
}
static int RaceStart(const unsigned char *pixels, unsigned width, unsigned height) {
    unsigned needle = 0, blueRoad = 0, mirrorRoad = 0, blueSky = 0, overpass = 0;
    int secondGlyphMatchesThird = 1;
    if (width != 320 || height != 240) return 0;
    for (unsigned y = 0; y < height; ++y) for (unsigned x = 0; x < width; ++x) {
        const unsigned char *p = pixels + ((size_t)y * width + x) * 3u;
        if (y >= 155 && y < 215 && x >= 250 && x < 300)
            needle += p[0] > 100 && p[0] > p[1] + 60 && p[0] > p[2] + 60;
        if (y >= 140 && y < 165 && x >= 60 && x < 260)
            blueRoad += p[0] < 8 && p[1] < 8 && p[2] > 35;
        if (y >= 18 && y < 54 && x >= 86 && x < 234)
            mirrorRoad += p[0] > 25 && abs((int)p[0] - (int)p[1]) < 12 && abs((int)p[0] - (int)p[2]) < 12;
        if (y >= 55 && y < 105 && x >= 100 && x < 220)
            blueSky += p[0] >= 20 && p[0] <= 80 && p[1] >= 50 && p[1] <= 125 && p[2] >= 100 && p[2] <= 190;
        if (y >= 90 && y < 126 && x < 140)
            overpass += p[0] >= 70 && p[0] <= 190 && p[1] >= 65 && p[1] <= 180 && p[2] >= 45 && p[2] <= 140 && p[0] >= p[2] + 15;
    }
    for (unsigned y = 148; y < 156 && secondGlyphMatchesThird; ++y) for (unsigned offset = 0; offset < 8; ++offset) {
        const unsigned char *second = pixels + ((size_t)y * width + 272 + offset) * 3u;
        const unsigned char *third = pixels + ((size_t)y * width + 280 + offset) * 3u;
        int secondOn = second[0] > 120 && second[1] > 80 && second[2] < 120;
        int thirdOn = third[0] > 120 && third[1] > 80 && third[2] < 120;
        if (secondOn != thirdOn) secondGlyphMatchesThird = 0;
    }
    if (secondGlyphMatchesThird || needle < 330 || blueRoad >= 128 || mirrorRoad < 3000 || blueSky < 2000 || overpass < 700)
        fprintf(stderr, "race-start image invalid: glyph=%d needle=%u blue_road=%u mirror=%u sky=%u overpass=%u\n", secondGlyphMatchesThird, needle, blueRoad, mirrorRoad, blueSky, overpass);
    return !secondGlyphMatchesThird && needle >= 330 && blueRoad < 128 && mirrorRoad >= 3000 && blueSky >= 2000 && overpass >= 700;
}
static int Fmv(const unsigned char *pixels, unsigned width, unsigned height) {
    unsigned pictureNonBlack = 0, greenCorruption = 0;
    if (width != 320 || height != 240) return 0;
    for (unsigned y = 0; y < height; ++y) for (unsigned x = 0; x < width; ++x) {
        const unsigned char *p = pixels + ((size_t)y * width + x) * 3u;
        if (y < 24 || y >= 216) {
            if (p[0] || p[1] || p[2]) {
                fprintf(stderr, "FMV is not vertically centered\n");
                return 0;
            }
        } else {
            pictureNonBlack += p[0] || p[1] || p[2];
            greenCorruption += p[1] > 60 && p[1] > p[0] + 40 && p[1] > p[2] + 30;
        }
    }
    if (pictureNonBlack < 10000 || greenCorruption > 2000)
        fprintf(stderr, "FMV image invalid: picture=%u green_corruption=%u\n", pictureNonBlack, greenCorruption);
    return pictureNonBlack >= 10000 && greenCorruption <= 2000;
}
int main(int argc, char **argv) {
    unsigned width = 0, height = 0;
    unsigned char *pixels;
    int ok;
    if (argc == 4 && !strcmp(argv[1], "title")) return Title(argv[2], argv[3]) ? 0 : 1;
    if (argc >= 4 && !strcmp(argv[1], "sky"))
        return SkyDigest(argc - 3, argv + 3, (uint32_t)strtoul(argv[2], NULL, 0)) ? 0 : 1;
    if (argc == 5 && !strcmp(argv[1], "nonempty"))
        return NonEmpty(argv[2], (unsigned)strtoul(argv[3], NULL, 10),
                        (unsigned)strtoul(argv[4], NULL, 10)) ? 0 : 1;
    if (argc != 3) return 2;
    pixels = ReadPPM(argv[2], &width, &height);
    if (!pixels) { fprintf(stderr, "invalid PPM: %s\n", argv[2]); return 1; }
    if (!strcmp(argv[1], "waterfall")) ok = Waterfall(pixels, width, height);
    else if (!strcmp(argv[1], "pegase")) ok = Pegase(pixels, width, height);
    else if (!strcmp(argv[1], "boot")) ok = Boot(pixels, width, height);
    else if (!strcmp(argv[1], "trophy")) ok = Trophy(pixels, width, height);
    else if (!strcmp(argv[1], "prologue")) ok = Prologue(pixels, width, height);
    else if (!strcmp(argv[1], "grand_prix")) ok = GrandPrix(pixels, width, height);
    else if (!strcmp(argv[1], "race_start")) ok = RaceStart(pixels, width, height);
    else if (!strcmp(argv[1], "fmv")) ok = Fmv(pixels, width, height);
    else return 2;
    free(pixels);
    return ok ? 0 : 1;
}
