#ifndef PORT_CAR_CUSTOM_H
#define PORT_CAR_CUSTOM_H
#include <stdint.h>

/* What a player adds to their car beyond its paint, as the retail DESIGN mode
 * does: a 64x64 logo (16-colour, on the bonnet) and a team name of up to six
 * characters (in the windscreen strip). Both live in the car's shared texture
 * page, so applying them means rewriting those rectangles of a copy of it. */
enum {
    CAR_LOGO_SIZE = 64,
    CAR_LOGO_BYTES = CAR_LOGO_SIZE * CAR_LOGO_SIZE / 2, /* 4 bits per pixel, the left pixel in the low nibble */
    CAR_LOGO_COLORS = 16,                                /* palette entry 0 is transparent */
    CAR_TAG_MAX = 6,
    CAR_SHARED_WORDS = 64 * 256,                         /* the shared page: 64 x 256 halfwords */
};

typedef struct CarCustom {
    int hasLogo;
    uint8_t logo[CAR_LOGO_BYTES];
    uint16_t clut[CAR_LOGO_COLORS]; /* PlayStation 15-bit colours */
    uint8_t tag[CAR_TAG_MAX];       /* font glyph indices */
    int tagLength;
    uint32_t hash;                  /* identifies the content; 0 when there is none */
} CarCustom;

/* Sets the team name from text (letters, digits and . - ! ? @; anything else is
 * skipped, at most CAR_TAG_MAX characters). */
void CarCustomSetTag(CarCustom *custom, const char *text);
/* Sets the logo: CAR_LOGO_BYTES of pixels and CAR_LOGO_COLORS palette entries. */
void CarCustomSetLogo(CarCustom *custom, const uint8_t *pixels, const uint16_t *clut);
void CarCustomClear(CarCustom *custom);
/* Rewrites the logo and name rectangles of a copy of the shared page, and the
 * logo's palette. */
void CarCustomApply(const CarCustom *custom, uint16_t shared[CAR_SHARED_WORDS],
                    uint16_t palette[CAR_LOGO_COLORS]);

#endif
