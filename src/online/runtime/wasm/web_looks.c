/* How each human seat's car looks: rw_set_look stages a seat's paint, logo
 * and name for the next race prepared; the running race's logos and names are
 * kept here for its textures to read; the paint catalogue for a colour picker;
 * and the garage preview's logo and name, changed live. */
#include <emscripten/emscripten.h>
#include <string.h>
#include "render/car_paint.h"
#include "web_looks.h"

static WebSeatLook s_nextLooks[DRIVER_SEAT_LIMIT];
static CarCustom s_raceCustom[DRIVER_SEAT_LIMIT];

/* A logo: CAR_LOGO_BYTES of 4-bit pixels, then CAR_LOGO_COLORS little-endian
 * 15-bit colours (palette entry 0 is transparent); NULL removes it. */
static void ReadLogo(CarCustom *custom, const uint8_t *data) {
    uint16_t clut[CAR_LOGO_COLORS];
    if (!data) {
        CarCustomSetLogo(custom, NULL, NULL);
        return;
    }
    for (int i = 0; i < CAR_LOGO_COLORS; ++i)
        clut[i] = (uint16_t)(data[CAR_LOGO_BYTES + i * 2] | (data[CAR_LOGO_BYTES + i * 2 + 1] << 8));
    CarCustomSetLogo(custom, data, clut);
}

/* A seat's look for the next race: paint (first and second colour, -1 for
 * the factory ones), logo (see ReadLogo; NULL for none) and windscreen name. */
EMSCRIPTEN_KEEPALIVE void rw_set_look(int seat, int first, int second, const uint8_t *logo, const char *tag) {
    if (seat < 0 || seat >= DRIVER_SEAT_LIMIT) return;
    WebSeatLook *look = &s_nextLooks[seat];
    const int valid = first >= 0 && first < RAGE_CAR_PAINT_COLOR_COUNT &&
                      second >= 0 && second < RAGE_CAR_PAINT_COLOR_COUNT;
    look->paint[0] = valid ? first : -1;
    look->paint[1] = valid ? second : -1;
    ReadLogo(&look->custom, logo);
    CarCustomSetTag(&look->custom, tag);
}

void WebLooksClear(void) {
    memset(s_nextLooks, 0, sizeof(s_nextLooks));
    for (int seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) s_nextLooks[seat].paint[0] = s_nextLooks[seat].paint[1] = -1;
}

void WebLooksTake(WebSeatLook looks[DRIVER_SEAT_LIMIT]) {
    memcpy(looks, s_nextLooks, sizeof(s_nextLooks));
    WebLooksClear();
}

void WebLooksUseInRace(const WebSeatLook looks[DRIVER_SEAT_LIMIT]) {
    for (int seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) s_raceCustom[seat] = looks[seat].custom;
}

const CarCustom *WebRaceCustom(int seat) {
    return seat >= 0 && seat < DRIVER_SEAT_LIMIT ? &s_raceCustom[seat] : NULL;
}

uint32_t WebRaceCustomHash(uint32_t seat) {
    return seat < DRIVER_SEAT_LIMIT ? s_raceCustom[seat].hash : 0;
}

/* The catalogue for a colour picker: how many colours, and one's RGB. */
EMSCRIPTEN_KEEPALIVE int rw_paint_count(void) { return RAGE_CAR_PAINT_COLOR_COUNT; }
EMSCRIPTEN_KEEPALIVE const uint8_t *rw_paint_swatch(int color) {
    static uint8_t rgb[3];
    return CarPaintSwatch((uint8_t)color, rgb) ? rgb : NULL;
}

/* The garage preview's logo (see ReadLogo) and team name, changed live. */
EMSCRIPTEN_KEEPALIVE void rw_set_showroom_logo(const uint8_t *data) { ReadLogo(&s_raceCustom[0], data); }
EMSCRIPTEN_KEEPALIVE void rw_set_showroom_tag(const char *text) { CarCustomSetTag(&s_raceCustom[0], text); }
