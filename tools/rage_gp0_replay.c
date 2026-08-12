#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libgpu.h>
#include <psyz/video.h>

static void usage(const char *program) {
    fprintf(stderr, "usage: %s VRAM.RAW GP0.LOG FRAME OUTPUT.PPM [LAST_PACKET [ONLY_PACKET]]\n",
            program);
}

static int append_words(const char *text, uint32_t **words, size_t *count,
                        size_t *capacity) {
    const char *cursor = text;
    while (*cursor && *cursor != '\n') {
        char *end;
        unsigned long value = strtoul(cursor, &end, 16);
        if (end == cursor || value > UINT32_MAX) return -1;
        if (*count == *capacity) {
            size_t next = *capacity ? *capacity * 2 : 4096;
            uint32_t *grown = realloc(*words, next * sizeof(**words));
            if (!grown) return -1;
            *words = grown;
            *capacity = next;
        }
        (*words)[(*count)++] = (uint32_t)value;
        cursor = end;
        if (*cursor == ',') cursor++;
        else break;
    }
    return 0;
}

int main(int argc, char **argv) {
    FILE *file;
    uint16_t *vram;
    uint32_t *words = NULL;
    size_t word_count = 0, word_capacity = 0;
    char line[65536];
    long wanted_frame;
    long last_packet = -1;
    long only_packet = -1;
    long packet_index = 0;
    char frame_field[64];
    RECT rect = {0, 0, 1024, 512};
    DRAWENV draw;
    DISPENV display;
    unsigned char *pixels;
    uint16_t *page;
    int width = 320, height = 240, page_y = 0, y;

    if (argc < 5 || argc > 7) { usage(argv[0]); return 2; }
    wanted_frame = strtol(argv[3], NULL, 0);
    if (argc == 6) last_packet = strtol(argv[5], NULL, 0);
    if (argc == 7) {
        last_packet = strtol(argv[5], NULL, 0);
        only_packet = strtol(argv[6], NULL, 0);
    }
    snprintf(frame_field, sizeof(frame_field), "frame=%ld ", wanted_frame);

    file = fopen(argv[1], "rb");
    if (!file) { perror(argv[1]); return 1; }
    vram = malloc(1024u * 512u * sizeof(*vram));
    if (!vram || fread(vram, sizeof(*vram), 1024u * 512u, file) != 1024u * 512u) {
        fprintf(stderr, "invalid 1024x512 RGB5551 VRAM snapshot: %s\n", argv[1]);
        fclose(file); free(vram); return 1;
    }
    fclose(file);

    if (Psyz_GpuReplayBegin() != 0) return 1;
    {
        const char *resolution = getenv("RAGE_GP0_REPLAY_INTERNAL_RES");
        if (resolution &&
            Psyz_VideoSetInternalResolution((unsigned)strtoul(resolution, NULL, 0)) != 0) {
            fprintf(stderr, "invalid replay internal resolution: %s\n", resolution);
            return 1;
        }
    }
    file = fopen(argv[2], "r");
    if (!file) { perror(argv[2]); free(vram); return 1; }
    while (fgets(line, sizeof(line), file)) {
        char *payload;
        if (!strstr(line, frame_field)) continue;
        payload = strstr(line, "words=");
        if (!payload) continue;
        if (append_words(payload + 6, &words, &word_count, &word_capacity) != 0) {
            fprintf(stderr, "invalid GP0 words in %s\n", argv[2]);
            fclose(file); free(vram); free(words); return 1;
        }
        if (page_y == 0) {
            size_t i;
            for (i = 0; i < word_count; i++) {
                if ((words[i] >> 24) == 0xe3) {
                    page_y = (int)((words[i] >> 10) & 0x3ff) / 240 * 240;
                    break;
                }
            }
        }
    }
    fclose(file);
    if (!word_count) {
        fprintf(stderr, "no GP0 words for frame %ld in %s\n", wanted_frame, argv[2]);
        free(vram); free(words); return 1;
    }

    ResetGraph(0);
    SetDefDrawEnv(&draw, 0, 0, 320, 240);
    SetDefDispEnv(&display, 0, 0, 320, 240);
    PutDrawEnv(&draw);
    PutDispEnv(&display);
    SetDispMask(1);
    LoadImage(&rect, (u_long *)vram);
    DrawSync(0);
    /* The log parser above retained a flat array for compactness. Re-read it
     * now and replay one canonical trace record at a time. */
    file = fopen(argv[2], "r");
    if (!file) return 1;
    while (fgets(line, sizeof(line), file)) {
        uint32_t *packet = NULL;
        size_t packet_count = 0, packet_capacity = 0;
        char *payload;
        int code;
        if (!strstr(line, frame_field) || !(payload = strstr(line, "words="))) continue;
        if (last_packet >= 0 && packet_index > last_packet) break;
        if (append_words(payload + 6, &packet, &packet_count, &packet_capacity) != 0) {
            fclose(file); free(packet); return 1;
        }
        code = packet_count ? (int)(packet[0] >> 24) : -1;
        if ((only_packet < 0 || packet_index == only_packet ||
             (code >= 0xe1 && code <= 0xe6)) &&
            Psyz_GpuReplayPacket(packet, (int)packet_count) != 0) {
            fclose(file); free(packet); return 1;
        }
        packet_index++;
        if ((word_count & 0x3ff) == 0) fflush(stderr);
        free(packet);
    }
    fclose(file);
    if (Psyz_GpuReplayEnd() != 0) return 1;
    DrawSync(0);
    page = malloc((size_t)width * height * sizeof(*page));
    pixels = malloc((size_t)width * height * 3);
    if (!page || !pixels) { fprintf(stderr, "capture allocation failed\n"); return 1; }
    rect = (RECT){0, (short)page_y, (short)width, (short)height};
    StoreImage(&rect, (u_long *)page);
    DrawSync(0);
    for (size_t i = 0; i < (size_t)width * height; i++) {
        uint16_t value = page[i];
        pixels[i * 3 + 0] = (unsigned char)((value & 31) * 255 / 31);
        pixels[i * 3 + 1] = (unsigned char)(((value >> 5) & 31) * 255 / 31);
        pixels[i * 3 + 2] = (unsigned char)(((value >> 10) & 31) * 255 / 31);
    }
    file = fopen(argv[4], "wb");
    if (!file) { perror(argv[4]); return 1; }
    fprintf(file, "P6\n%d %d\n255\n", width, height);
    for (y = 0; y < height; y++)
        fwrite(pixels + (size_t)y * width * 3, 3, (size_t)width, file);
    fclose(file);
    fprintf(stderr, "replayed %zu GP0 words to %s (%dx%d)\n",
            word_count, argv[4], width, height);
    free(page); free(pixels); free(vram); free(words);
    ResetGraph(0);
    return 0;
}
