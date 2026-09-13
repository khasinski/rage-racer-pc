#ifndef RAGE_MENU_MUSIC_RENDER_H
#define RAGE_MENU_MUSIC_RENDER_H

#include "menu_music_asset.h"

unsigned MenuMusicTickRate(const MenuMusicAsset *asset);
int MenuMusicRenderWav(const MenuMusicAsset *asset, unsigned tickRate,
                       const char *outputPath);

#endif
