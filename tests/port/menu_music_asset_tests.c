#include "menu_music_asset.h"

#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(value) do { if (!(value)) { failures++; \
    fprintf(stderr, "line %d: %s\n", __LINE__, #value); } } while (0)

static void Write32Le(unsigned char *p, unsigned value) {
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
    p[2] = (unsigned char)(value >> 16);
    p[3] = (unsigned char)(value >> 24);
}

static void CheckRegionTempo(unsigned tempo, unsigned expected) {
    unsigned char data[48] = {0};
    MenuMusicAsset asset;

    Write32Le(data, 12);
    Write32Le(data + 4, 24);
    Write32Le(data + 8, 40);
    memcpy(data + 24, "pQES", 4);
    data[31] = 1;
    data[32] = 1;
    data[33] = 0xe0;
    data[34] = (unsigned char)(tempo >> 16);
    data[35] = (unsigned char)(tempo >> 8);
    data[36] = (unsigned char)tempo;

    CHECK(MenuMusicAssetOpen(data, sizeof(data), &asset));
    CHECK(asset.header.size == 12 && asset.sequence.size == 16);
    CHECK(asset.samples.size == 8 && asset.division == 480);
    CHECK(asset.tempoUs == expected);
}

static void CheckSequenceTicks(void) {
    unsigned char data[80] = {0};
    unsigned char *seq = data + 24;
    MenuMusicAsset asset;
    size_t ticks = 0;

    Write32Le(data, 12);
    Write32Le(data + 4, 24);
    Write32Le(data + 8, 43);
    memcpy(seq, "pQES", 4);
    seq[7] = 1;
    seq[8] = 0;
    seq[9] = 60;       /* 60 MIDI ticks per quarter note. */
    seq[10] = 0x0f;
    seq[11] = 0x42;
    seq[12] = 0x40;    /* 1 second per quarter note. */
    seq[15] = 60;      /* EOT occurs after one quarter note. */
    seq[16] = 0xff;
    seq[17] = 0x2f;
    seq[18] = 0;

    CHECK(MenuMusicAssetOpen(data, sizeof(data), &asset));
    CHECK(MenuMusicSequenceTicks(&asset, 60, &ticks));
    CHECK(ticks == 60);
    CHECK(MenuMusicSequenceTicks(&asset, 50, &ticks));
    CHECK(ticks == 50);

    seq[18] = 4;       /* Truncated EOT payload. */
    CHECK(!MenuMusicSequenceTicks(&asset, 60, &ticks));
}

int main(void) {
    unsigned char invalid[48] = {0};
    MenuMusicAsset asset;

    CheckRegionTempo(0x0504f3, 328947); /* PAL disc */
    CheckRegionTempo(0x0605f0, 394736); /* NTSC-U/J discs */
    CheckSequenceTicks();
    CHECK(!MenuMusicAssetOpen(NULL, 0, &asset));
    CHECK(!MenuMusicAssetOpen(invalid, sizeof(invalid), &asset));
    Write32Le(invalid, 12);
    Write32Le(invalid + 4, 40);
    Write32Le(invalid + 8, 24);
    CHECK(!MenuMusicAssetOpen(invalid, sizeof(invalid), &asset));
    return failures != 0;
}
