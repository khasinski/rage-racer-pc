/* The retail race tachometer, for the browser to draw over the 3D view.
 *
 * Retail (car/player_tachometer.c DrawPlayerTachometer, race/draw_tachometer.c
 * DrawTachometer, race/draw_speed_digits.c, race/draw_hud_digit.c) builds it
 * from GPU packets each game frame:
 *   - a flat POLY_F4 needle turned by the engine rpm,
 *   - the car's own dial face, a 96x94 4-bit SPRT (render/tachometer_needle.c,
 *     g_TachoNeedleSprite) on texture page 0xA, drawn with a normal or a dark
 *     CLUT and a brightness that fades with the time of day,
 *   - three speed digits and the gear digit, 8x8 SPRTs from texture page 9,
 *     green for a manual gearbox and yellow for an automatic one,
 *   - a 16x16 TILE shift light behind the face.
 * The images come from the boot car screen asset (HUD page at VRAM 576,0 and
 * the digit CLUTs at rows 480/483) and the car's race pack (its face at
 * 640,112 with CLUTs at rows 206/207), exactly what retail uploads.
 * This file decodes those sprites once per race and evaluates the packet
 * fields per frame from the simulation and its presentation (engine sound
 * state, environment clock, track zone); the browser only rasterises them. */
#include "web_hud.h"

#include <string.h>

#include "game/angle.h"
#include "game/integer.h"
#include "game/asset_index.h"
#include "game/image_asset.h"
#include "game/track.h"
#include "native_texture.h"

enum {
    /* Texture pages and CLUTs as the retail packets name them. */
    HUD_DIGIT_TPAGE = 9,
    HUD_FACE_TPAGE = 0xA,
    HUD_FACE_CLUT = 0x33A8,
    HUD_FACE_CLUT_DARK = 0x33E8,
    HUD_DIGIT_CLUT_MANUAL = 0x7800,
    HUD_DIGIT_CLUT_AUTOMATIC = 0x78CF,
    /* g_TachoNeedleSprite. */
    HUD_FACE_U = 0,
    HUD_FACE_V = 0x70,
    HUD_FACE_WIDTH = 0x60,
    HUD_FACE_HEIGHT = 0x5E,
    /* DrawHudDigit. */
    HUD_DIGIT_SIZE = 8,
    HUD_DIGIT_V = 0x10,
    /* The VRAM the sprites read: pages 9 and 0xA with the face CLUTs, and
     * the rows holding the digit CLUTs. */
    HUD_PAGE_X = 576,
    HUD_PAGE_WIDTH = 128,
    HUD_PAGE_HEIGHT = 256,
    HUD_CLUT_Y = 480,
    HUD_CLUT_WIDTH = 256,
    HUD_CLUT_HEIGHT = 4,
    /* Atlas layout: both faces side by side, then the two digit strips. */
    ATLAS_DIGITS_Y = 96,
};

/* player_tachometer.c. */
enum {
    TACHOMETER_DARK_ZONE = 3,
    DAWN_FADE_START = 0x1154,
    DAYLIGHT_START = 0x11D4,
    DUSK_FADE_START = 0x5420,
    NIGHT_START = 0x54A0,
};

/* draw_tachometer.c. */
enum {
    TACHOMETER_BLEND_FRAMES = 96,
    TACHOMETER_DARK_LEVEL = 32,
    TACHOMETER_NORMAL_LEVEL = 128,
    TACHOMETER_MAX_RPM = 10000,
};

typedef enum Lighting { LIGHTING_NORMAL, LIGHTING_DARK, LIGHTING_FADE_FROM_DARK, LIGHTING_FADE_TO_DARK } Lighting;

/* Layout of the per-frame words (WebHudTachometer). */
enum {
    T_VISIBLE = 0,
    T_NEEDLE = 1,        /* 4 x (x, y), POLY_F4 vertex order */
    T_NEEDLE_COLOR = 9,  /* r, g, b */
    T_FACE = 12,         /* x, y (top left), brightness (128 = unmodulated), dark CLUT */
    T_GEAR = 16,         /* x, y, digit */
    T_SPEED = 19,        /* x, y, value 0..999 (three digits) */
    T_DIGIT_CLUT = 22,   /* 0 manual (green), 1 automatic (yellow) */
    T_SHIFT_LIGHT = 23,  /* x, y, red (green and blue are 32) */
    T_FACE_SIZE = 26,    /* width, height */
    T_WORDS = 28,
};
_Static_assert(T_WORDS <= WEB_HUD_TACHO_WORDS, "tachometer words overflow");

static uint8_t s_atlas[WEB_HUD_ATLAS_WIDTH * WEB_HUD_ATLAS_HEIGHT * 4];
static int32_t s_tacho[WEB_HUD_TACHO_WORDS];
static int s_ready, s_mph;
/* The face CLUT stays in the frame's packet between frames: fading towards
 * night leaves whichever one the previous frames set. */
static int s_faceDark;

static void Decode(const TextureImage *vram, uint16_t tpage, uint16_t clut, int u0, int v0,
                   int width, int height, int atlasX, int atlasY) {
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            TextureColor(TextureWord(vram, tpage, clut, (uint32_t)(u0 + x), (uint32_t)(v0 + y)),
                         &s_atlas[((size_t)(atlasY + y) * WEB_HUD_ATLAS_WIDTH + atlasX + x) * 4]);
}

int WebHudPrepare(const RaceData *archive, int variant) {
    static uint16_t page[HUD_PAGE_WIDTH * HUD_PAGE_HEIGHT], cluts[HUD_CLUT_WIDTH * HUD_CLUT_HEIGHT];
    const ImagePixels pageImage = {page, HUD_PAGE_WIDTH * HUD_PAGE_HEIGHT, HUD_PAGE_X, 0,
                                   HUD_PAGE_WIDTH, HUD_PAGE_HEIGHT};
    const ImagePixels clutImage = {cluts, HUD_CLUT_WIDTH * HUD_CLUT_HEIGHT, 0, HUD_CLUT_Y,
                                   HUD_CLUT_WIDTH, HUD_CLUT_HEIGHT};
    const TextureImage clutView = {cluts, HUD_CLUT_WIDTH * HUD_CLUT_HEIGHT, 0, HUD_CLUT_Y,
                                   HUD_CLUT_WIDTH, HUD_CLUT_HEIGHT, NULL};
    const TextureImage vram = {page, HUD_PAGE_WIDTH * HUD_PAGE_HEIGHT, HUD_PAGE_X, 0,
                               HUD_PAGE_WIDTH, HUD_PAGE_HEIGHT, &clutView};
    const void *boot, *car;
    size_t bootSize = 0, carSize = 0;
    RaceCarAssetHeader header;

    s_ready = 0;
    if ((unsigned)variant >= CAR_MODEL_VARIANT_COUNT) return 0;
    boot = RaceAsset(archive, ASSET_BOOT_CAR_SCREEN, &bootSize);
    car = RaceAsset(archive, CarVariantAssetIndex(ASSET_CAR_2ND_BASE, variant), &carSize);
    if (!boot || !car || carSize < sizeof(header)) return 0;
    memcpy(&header, car, sizeof(header));
    if (header.imageOffset <= 0 || (size_t)header.imageOffset >= carSize) return 0;
    memset(page, 0, sizeof(page));
    memset(cluts, 0, sizeof(cluts));
    /* Boot resources first, then the race pack's image (race_assets.c). */
    if (!CopyImageAssetPixels(boot, bootSize, &pageImage) ||
        !CopyImageAssetPixels(boot, bootSize, &clutImage) ||
        !CopyImageAssetPixels((const u8 *)car + header.imageOffset,
                              carSize - (size_t)header.imageOffset, &pageImage)) return 0;

    memset(s_atlas, 0, sizeof(s_atlas));
    Decode(&vram, HUD_FACE_TPAGE, HUD_FACE_CLUT, HUD_FACE_U, HUD_FACE_V,
           HUD_FACE_WIDTH, HUD_FACE_HEIGHT, 0, 0);
    Decode(&vram, HUD_FACE_TPAGE, HUD_FACE_CLUT_DARK, HUD_FACE_U, HUD_FACE_V,
           HUD_FACE_WIDTH, HUD_FACE_HEIGHT, HUD_FACE_WIDTH, 0);
    Decode(&vram, HUD_DIGIT_TPAGE, HUD_DIGIT_CLUT_MANUAL, 0, HUD_DIGIT_V,
           10 * HUD_DIGIT_SIZE, HUD_DIGIT_SIZE, 0, ATLAS_DIGITS_Y);
    Decode(&vram, HUD_DIGIT_TPAGE, HUD_DIGIT_CLUT_AUTOMATIC, 0, HUD_DIGIT_V,
           10 * HUD_DIGIT_SIZE, HUD_DIGIT_SIZE, 0, ATLAS_DIGITS_Y + HUD_DIGIT_SIZE);
    /* speed_display.c: the NTSC-U release shows mph. */
    s_mph = strncmp(archive->boot, "SLUS", 4) == 0;
    s_faceDark = 0;
    s_ready = 1;
    return 1;
}

const uint8_t *WebHudAtlas(void) { return s_ready ? s_atlas : NULL; }

/* speed_display.c SpeedDisplayValue. */
static s32 SpeedDisplay(s32 speed) {
    int64_t value;
    if (speed <= 0) return 0;
    value = (int64_t)speed * 160 / 1168;
    if (s_mph) value = value * 100 / 160;
    return value < 999 ? (s32)value : 999;
}

static s32 ClampBlend(s32 amount) {
    return amount < 0 ? 0 : amount > TACHOMETER_BLEND_FRAMES ? TACHOMETER_BLEND_FRAMES : amount;
}

static s32 BlendChannel(s32 from, s32 to, s32 amount) {
    return (u8)((from * (TACHOMETER_BLEND_FRAMES - amount) + to * amount) / TACHOMETER_BLEND_FRAMES);
}

/* draw_tachometer.c SetTachometerNeedleColor: the needle colour, the face
 * CLUT and the face brightness for the lighting. */
static s32 NeedleColor(const CarTachometerSpec *spec, Lighting lighting, s32 amount, int32_t rgb[3]) {
    s32 brightness = TACHOMETER_NORMAL_LEVEL;
    if (lighting == LIGHTING_FADE_TO_DARK) {
        amount = ClampBlend(amount);
        brightness = TACHOMETER_NORMAL_LEVEL - amount;
        for (int c = 0; c < 3; ++c) rgb[c] = BlendChannel(spec->needleColor[c], TACHOMETER_DARK_LEVEL, amount);
    } else if (lighting == LIGHTING_FADE_FROM_DARK) {
        amount = amount <= TACHOMETER_DARK_LEVEL ? 0 : ClampBlend(amount - TACHOMETER_DARK_LEVEL);
        brightness = TACHOMETER_DARK_LEVEL + amount;
        for (int c = 0; c < 3; ++c) rgb[c] = BlendChannel(TACHOMETER_DARK_LEVEL, spec->needleColor[c], amount);
        s_faceDark = 0;
    } else if (lighting == LIGHTING_DARK) {
        s_faceDark = 1;
        for (int c = 0; c < 3; ++c) rgb[c] = spec->needleColorAlt[c];
    } else {
        s_faceDark = 0;
        for (int c = 0; c < 3; ++c) rgb[c] = spec->needleColor[c];
    }
    return (u8)brightness;
}

const int32_t *WebHudTachometer(const ClientRace *race, int seat) {
    const SimDriver *driver;
    const CarTachometerSpec *spec;
    const EngineSound *engine;
    TrackZoneEffect zone;
    Lighting lighting;
    s32 amount, rpm, angle, sine, cosine, centerX, centerY, clock;

    memset(s_tacho, 0, sizeof(s_tacho));
    if (!s_ready || !race || seat < 0 || seat >= DRIVER_SEAT_LIMIT) return s_tacho;
    driver = &race->sim.drivers[seat];
    /* race_scene.c draws it from the countdown on, not while retired. */
    if (race->sim.phase < SIM_COUNTDOWN ||
        (driver->status != SIM_DRIVING && driver->status != SIM_DRIVER_FINISHED)) return s_tacho;
    spec = &driver->spec.tachometer;
    engine = &race->view->engines[seat];

    /* player_tachometer.c DrawPlayerTachometer. */
    zone = ReadTrackZoneEffect(race->sim.events, driver->car.trackProgress,
                               race->sim.route.length, race->sim.reverse);
    clock = race->env.clock;
    if (zone.dark == TACHOMETER_DARK_ZONE || clock < DAWN_FADE_START || clock >= NIGHT_START) {
        lighting = LIGHTING_DARK;
        amount = 0;
    } else if (clock < DAYLIGHT_START) {
        lighting = LIGHTING_FADE_FROM_DARK;
        amount = clock - DAWN_FADE_START;
    } else if (clock < DUSK_FADE_START) {
        lighting = LIGHTING_NORMAL;
        amount = 0;
    } else {
        lighting = LIGHTING_FADE_TO_DARK;
        amount = clock - DUSK_FADE_START;
    }
    rpm = WrapSigned32((int64_t)engine->rpm + engine->jitter);

    /* draw_tachometer.c DrawTachometer. */
    centerX = spec->needleX;
    centerY = spec->needleY;
    if (rpm < 0) rpm = 0;
    if (rpm > TACHOMETER_MAX_RPM) rpm = TACHOMETER_MAX_RPM;
    angle = spec->angleMin + rpm * (spec->angleMax - spec->angleMin) / TACHOMETER_MAX_RPM;
    sine = CosAngle(angle);
    cosine = SinAngle(angle);
    for (int i = 0; i < 4; ++i) {
        const s32 width = spec->needleQuad[(i & 1) ? 1 : 3];
        const s32 localX = WrapSigned16(i < 2 ? -width : width);
        const s32 localY = WrapSigned16((i & 1) ? -spec->needleQuad[0] : spec->needleQuad[2]);
        s_tacho[T_NEEDLE + i * 2] =
            WrapSigned16(centerX + ((int64_t)sine * localX - (int64_t)cosine * localY) / 4096);
        s_tacho[T_NEEDLE + i * 2 + 1] =
            WrapSigned16(centerY + ((int64_t)cosine * localX + (int64_t)sine * localY) / 4096);
    }
    s_tacho[T_FACE + 2] = NeedleColor(spec, lighting, amount, &s_tacho[T_NEEDLE_COLOR]);
    s_tacho[T_FACE] = WrapSigned16(spec->needleX + spec->faceDX);
    s_tacho[T_FACE + 1] = WrapSigned16(spec->needleY + spec->faceDY);
    s_tacho[T_FACE + 3] = s_faceDark;
    s_tacho[T_GEAR] = WrapSigned16(centerX + spec->gearDigitDX);
    s_tacho[T_GEAR + 1] = WrapSigned16(centerY + spec->gearDigitDY);
    s_tacho[T_GEAR + 2] = driver->car.drive.gear < 0 ? 0 : driver->car.drive.gear > 9 ? 9
                                                          : driver->car.drive.gear;
    s_tacho[T_SPEED] = WrapSigned16(WrapSigned32((int64_t)centerX + spec->digitsX));
    s_tacho[T_SPEED + 1] = WrapSigned16(WrapSigned32((int64_t)centerY + spec->digitsY));
    s_tacho[T_SPEED + 2] = SpeedDisplay(driver->car.speed);
    s_tacho[T_DIGIT_CLUT] = driver->car.drive.manual ? 0 : 1;
    s_tacho[T_SHIFT_LIGHT] = WrapSigned16((int64_t)centerX + spec->shiftLightDX);
    s_tacho[T_SHIFT_LIGHT + 1] = WrapSigned16((int64_t)centerY + spec->shiftLightDY);
    s_tacho[T_SHIFT_LIGHT + 2] = engine->shiftLight ? 255 : 32;
    s_tacho[T_FACE_SIZE] = HUD_FACE_WIDTH;
    s_tacho[T_FACE_SIZE + 1] = HUD_FACE_HEIGHT;
    s_tacho[T_VISIBLE] = 1;
    return s_tacho;
}
