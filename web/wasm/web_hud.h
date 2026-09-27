/* The retail tachometer for the browser (web_hud.c). */
#ifndef WEB_HUD_H
#define WEB_HUD_H
#include "client_race.h"
#include "game/race_data.h"

/* The sprite atlas the browser draws the tachometer from: RGBA, straight
 * alpha, WEB_HUD_ATLAS_WIDTH x WEB_HUD_ATLAS_HEIGHT. */
enum {
    WEB_HUD_ATLAS_WIDTH = 256,
    WEB_HUD_ATLAS_HEIGHT = 112,
    WEB_HUD_TACHO_WORDS = 32,
};

/* Decodes the local car's tachometer face and the HUD digits from the disc,
 * as retail has them in VRAM during a race. Returns 0 when the disc lacks
 * them. */
int WebHudPrepare(const RaceData *archive, int variant);
const uint8_t *WebHudAtlas(void);

/* One game frame of DrawPlayerTachometer for `seat`, in 320x240 PAL screen
 * coordinates (see web_hud.c for the layout of the words). */
const int32_t *WebHudTachometer(const ClientRace *race, int seat);
#endif
