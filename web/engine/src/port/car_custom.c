#include "car_custom.h"
#include <string.h>

extern const uint8_t g_CarTagFont[2688];

enum {
    /* Where retail's design mode uploads them, in the shared page (VRAM x 640..703). */
    SHARED_X = 640,
    LOGO_X = 656, LOGO_Y = 48, LOGO_HALFWORDS = 16,
    TAG_X = 0x282, TAG_Y = 0x37, TAG_HALFWORDS = 12, TAG_ROWS = 8,
    GLYPH_HALFWORDS = 2, GLYPH_BYTES = 32,
    GLYPH_COUNT = 84,
    /* Font layout: digits, blank, A..Z, then . - ! ? @ */
    GLYPH_BLANK = 10, GLYPH_LETTER = 11, GLYPH_DOT = 37, GLYPH_DASH = 38,
    GLYPH_BANG = 39, GLYPH_QUESTION = 40, GLYPH_AT = 41,
    BLANK_HALFWORD = 0xEEEE, /* palette entry 14 everywhere: the strip's own colour */
};

static uint32_t Mix(uint32_t hash, uint32_t value) { return (hash ^ value) * 16777619u; }

static void Rehash(CarCustom *custom) {
    uint32_t hash = 2166136261u;
    if (custom->hasLogo) {
        for (int i = 0; i < CAR_LOGO_BYTES; ++i) hash = Mix(hash, custom->logo[i]);
        for (int i = 0; i < CAR_LOGO_COLORS; ++i) hash = Mix(hash, custom->clut[i]);
    }
    for (int i = 0; i < custom->tagLength; ++i) hash = Mix(hash, 0x100u + custom->tag[i]);
    custom->hash = custom->hasLogo || custom->tagLength ? (hash | 1u) : 0;
}

void CarCustomClear(CarCustom *custom) { memset(custom, 0, sizeof(*custom)); }

void CarCustomSetTag(CarCustom *custom, const char *text) {
    custom->tagLength = 0;
    for (; text && *text && custom->tagLength < CAR_TAG_MAX; ++text) {
        char c = *text;
        int glyph = -1;
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        if (c >= '0' && c <= '9') glyph = c - '0';
        else if (c >= 'A' && c <= 'Z') glyph = GLYPH_LETTER + (c - 'A');
        else if (c == ' ' || c == '_') glyph = GLYPH_BLANK;
        else if (c == '.') glyph = GLYPH_DOT;
        else if (c == '-') glyph = GLYPH_DASH;
        else if (c == '!') glyph = GLYPH_BANG;
        else if (c == '?') glyph = GLYPH_QUESTION;
        else if (c == '@') glyph = GLYPH_AT;
        if (glyph >= 0) custom->tag[custom->tagLength++] = (uint8_t)glyph;
    }
    /* Blanks at either end would only shift the name off centre. */
    while (custom->tagLength && custom->tag[custom->tagLength - 1] == GLYPH_BLANK) --custom->tagLength;
    int lead = 0;
    while (lead < custom->tagLength && custom->tag[lead] == GLYPH_BLANK) ++lead;
    if (lead) {
        memmove(custom->tag, custom->tag + lead, (size_t)(custom->tagLength - lead));
        custom->tagLength -= lead;
    }
    Rehash(custom);
}

void CarCustomSetLogo(CarCustom *custom, const uint8_t *pixels, const uint16_t *clut) {
    custom->hasLogo = pixels != NULL && clut != NULL;
    if (custom->hasLogo) {
        memcpy(custom->logo, pixels, sizeof(custom->logo));
        memcpy(custom->clut, clut, sizeof(custom->clut));
    }
    Rehash(custom);
}

void CarCustomApply(const CarCustom *custom, uint16_t shared[CAR_SHARED_WORDS],
                    uint16_t palette[CAR_LOGO_COLORS]) {
    if (!custom) return;
    if (custom->hasLogo) {
        for (int row = 0; row < CAR_LOGO_SIZE; ++row) {
            for (int word = 0; word < LOGO_HALFWORDS; ++word) {
                const uint8_t *pair = &custom->logo[(row * LOGO_HALFWORDS + word) * 2];
                shared[(LOGO_Y + row) * 64 + (LOGO_X - SHARED_X) + word] = (uint16_t)(pair[0] | (pair[1] << 8));
            }
        }
        memcpy(palette, custom->clut, sizeof(custom->clut));
    }
    if (!custom->tagLength) return;
    for (int row = 0; row < TAG_ROWS; ++row)
        for (int word = 0; word < TAG_HALFWORDS; ++word)
            shared[(TAG_Y + row) * 64 + (TAG_X - SHARED_X) + word] = BLANK_HALFWORD;
    /* The name is centred in its twelve halfwords. */
    const int left = TAG_X - SHARED_X + (CAR_TAG_MAX - custom->tagLength);
    for (int i = 0; i < custom->tagLength; ++i) {
        const int glyph = custom->tag[i];
        if (glyph >= GLYPH_COUNT) continue;
        for (int row = 0; row < TAG_ROWS; ++row) {
            const uint8_t *bytes = &g_CarTagFont[glyph * GLYPH_BYTES + row * GLYPH_HALFWORDS * 2];
            for (int word = 0; word < GLYPH_HALFWORDS; ++word)
                shared[(TAG_Y + row) * 64 + left + i * GLYPH_HALFWORDS + word] =
                    (uint16_t)(bytes[word * 2] | (bytes[word * 2 + 1] << 8));
        }
    }
}
