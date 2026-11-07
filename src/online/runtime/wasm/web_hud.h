/* The retail tachometer for the browser (web_hud.c). */
#ifndef WEB_HUD_H
#define WEB_HUD_H
#include "client_race.h"
#include "game/race_data.h"
#include "../../../render/retail_tachometer.h"

/* The sprite atlas the browser draws the tachometer from: RGBA, straight
 * alpha, WEB_HUD_ATLAS_WIDTH x WEB_HUD_ATLAS_HEIGHT. */
enum {
    WEB_HUD_ATLAS_WIDTH = RAGE_TACHO_ATLAS_WIDTH,
    WEB_HUD_ATLAS_HEIGHT = RAGE_TACHO_ATLAS_HEIGHT,
    WEB_HUD_TACHO_WORDS = RAGE_TACHO_WORDS,
};

/* Decodes the local car's tachometer face and the HUD digits from the disc,
 * as retail has them in VRAM during a race. Returns 0 when the disc lacks
 * them. */
int WebHudPrepare(const RaceData *archive, int variant);
const uint8_t *WebHudAtlas(void);
uint32_t WebHudRevision(void);
/* Native unit conversion uses the same verified face+CLUT delta as offline.
 * Return variant+1 on success; zero means no prepared atlas. */
int WebHudUnitSnapshot(void *bytes,int size);
int WebHudApplyUnitSnapshot(const void *bytes,int size,int imperial);

/* One game frame of DrawPlayerTachometer for `seat`, in 320x240 PAL screen
 * coordinates (see web_hud.c for the layout of the words). */
const int32_t *WebHudTachometer(const ClientRace *race, int seat);
#endif
