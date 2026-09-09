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
    file = NULL;
    return pixels;
fail:
    if (file) fclose(file);
    free(pixels);
    return NULL;
}
int main(int argc, char **argv) {
    char manifest[4096], line[65536];
    FILE *file = NULL;
    unsigned rows = 0;
    int detailedManifest;
    if (argc != 2 || snprintf(manifest, sizeof(manifest), "%s/capture-manifest.csv", argv[1]) >= (int)sizeof(manifest) ||
        !(file = fopen(manifest, "rb")) || !fgets(line, sizeof(line), file)) goto fail;
    detailedManifest = !strncmp(line, "filename,capture_surface,frame,scene,timer,", 43);
    if (!detailedManifest && strcmp(line, "filename,frame,scene,timer\n")) goto fail;
    while (fgets(line, sizeof(line), file)) {
        char name[512], surface[64], path[4096];
        int frame, scene, timer;
        unsigned width, height, blue = 0;
        unsigned char *pixels = NULL;
        if ((detailedManifest ? sscanf(line, "%511[^,],%63[^,],%d,%d,%d", name, surface,
                                       &frame, &scene, &timer) :
                                sscanf(line, "%511[^,],%d,%d,%d", name, &frame, &scene, &timer)) !=
                (detailedManifest ? 5 : 4) ||
            rows >= 7 || scene != 12 || timer != 365 + (int)rows ||
            snprintf(path, sizeof(path), "%s/%s", argv[1], name) >= (int)sizeof(path) ||
            !(pixels = ReadPPM(path, &width, &height)) || width != 320 || height != 240) {
            free(pixels);
            goto fail;
        }
        for (size_t i = 0; i < (size_t)width * height; ++i) {
            const unsigned char *pixel = pixels + i * 3;
            blue += pixel[2] > 40 && pixel[2] > pixel[0] + 20 && pixel[2] > pixel[1] + 20;
        }
        free(pixels);
        if (blue > 20000) {
            fprintf(stderr, "mirror entry flooded timer %d blue (%u pixels)\n", timer, blue);
            goto fail;
        }
        ++rows;
        (void)frame;
    }
    fclose(file);
    if (rows == 7) return 0;
fail:
    if (file) fclose(file);
    fprintf(stderr, "mirror-entry capture must cover timers 365 through 371\n");
    return 1;
}
