#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *ReadFile(const char *path, size_t *size) {
    FILE *file = fopen(path, "rb");
    char *text;
    long length;
    if (!file || fseek(file, 0, SEEK_END) || (length = ftell(file)) < 0 ||
        fseek(file, 0, SEEK_SET)) goto fail;
    text = malloc((size_t)length + 1);
    if (!text || fread(text, 1, (size_t)length, file) != (size_t)length) goto fail;
    fclose(file); text[length] = 0; *size = (size_t)length;
    return text;
fail:
    if (file) fclose(file);
    return NULL;
}
int main(int argc, char **argv) {
    size_t firstSize, secondSize;
    char *first, *second, *line;
    unsigned rows = 0, race = 0, faces = 0, models = 0, terrain = 0, skipped = 0, packets = 0;
    if (argc != 3 || !(first = ReadFile(argv[1], &firstSize)) ||
        !(second = ReadFile(argv[2], &secondSize))) return 2;
    if (firstSize != secondSize || memcmp(first, second, firstSize)) {
        fprintf(stderr, "scene capture is not deterministic\n"); free(second); free(first); return 1;
    }
    for (line = strtok(first, "\n"); line; line = strtok(NULL, "\n")) {
        int frame, scene, timer, draws, terrainCount, cells, packetCount, faceCount, skippedCount;
        int overflow[5], used = 0; char hash[17];
        int fields = sscanf(line, "scene-frame frame=%d scene=%d timer=%d draws=%d terrain=%d cells=%d packets=%d faces=%d skipped3d=%d overflow=%d,%d,%d,%d,%d hash=%16[0-9a-f]%n",
            &frame, &scene, &timer, &draws, &terrainCount, &cells, &packetCount,
            &faceCount, &skippedCount, &overflow[0], &overflow[1], &overflow[2],
            &overflow[3], &overflow[4], hash, &used);
        if (fields != 15 || line[used]) { fprintf(stderr, "malformed scene-frame: %s\n", line); goto fail; }
        (void)frame; (void)timer;
        if (overflow[0] || overflow[1] || overflow[2] || overflow[3] || overflow[4]) {
            fprintf(stderr, "scene capture overflow at frame %d\n", frame); goto fail;
        }
        ++rows; packets += packetCount > 0;
        if (scene == 32) {
            ++race; faces += faceCount >= 200; models += draws >= 5;
            terrain += terrainCount >= 1 && cells >= 32; skipped += skippedCount >= 100;
        }
    }
    if (rows < 1200 || !race || faces < 100 || models < 100 || terrain < 100 || skipped < 100 || !packets) {
        fprintf(stderr, "scene capture insufficient: rows=%u race=%u faces=%u models=%u terrain=%u skipped=%u packets=%u\n", rows, race, faces, models, terrain, skipped, packets); goto fail;
    }
    free(second); free(first); return 0;
fail:
    free(second); free(first); return 1;
}
