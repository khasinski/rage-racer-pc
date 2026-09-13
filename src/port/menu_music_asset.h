#ifndef RAGE_MENU_MUSIC_ASSET_H
#define RAGE_MENU_MUSIC_ASSET_H

#include <stddef.h>
#include <stdint.h>

typedef struct MenuMusicSpan {
    const uint8_t *data;
    size_t size;
} MenuMusicSpan;

typedef struct MenuMusicAsset {
    MenuMusicSpan header;
    MenuMusicSpan sequence;
    MenuMusicSpan samples;
    uint32_t division;
    uint32_t tempoUs;
} MenuMusicAsset;

int MenuMusicAssetOpen(const void *data, size_t size, MenuMusicAsset *asset);

#endif
