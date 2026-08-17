#include "common.h"
#include "game/menu.h"
#include "game/asset_internal.h"
#include "game/save_internal.h"

static inline s32 TeamLogoParity(s32 value)
{
    return value & 1;
}

static inline s32 TeamLogoRowByteOffset(s32 row)
{
    return (row * 33) << 6;
}

static inline s32 TeamLogoParityByteOffset(s32 parity)
{
    return parity * 32;
}

static inline u16 *TeamLogoPaletteAddress(
    TeamLogoSample *samples, s32 row, s32 parity)
{
    TeamLogoSampleAddress baseAddress;
    TeamLogoSampleAddress address;
    TeamLogoSampleAddress parityAddress;
    s32 parityOffset;

    baseAddress.samplePointer = samples;
    address.offset = TeamLogoRowByteOffset(row);
    address.value = address.offset + baseAddress.value;
    parityOffset = TeamLogoParityByteOffset(parity);
    parityAddress.offset = parityOffset;
    parityAddress.value = parityAddress.offset + address.value;
    return parityAddress.halfwordPointer;
}

static inline u16 *TeamLogoClutAddress(
    TeamLogoSample *samples, s32 row, s32 parity, s32 index)
{
    TeamLogoSampleAddress baseAddress;
    TeamLogoSampleAddress address;
    TeamLogoSampleAddress parityAddress;
    TeamLogoSampleAddress elementAddress;
    s32 byteOffset;
    s32 parityOffset;

    byteOffset = index * 2;
    baseAddress.samplePointer = samples;
    address.offset = TeamLogoRowByteOffset(row);
    address.value = address.offset + baseAddress.value;
    parityOffset = TeamLogoParityByteOffset(parity);
    parityAddress.offset = parityOffset;
    parityAddress.value = parityAddress.offset + address.value;
    elementAddress.offset = byteOffset;
    elementAddress.value = elementAddress.offset + parityAddress.value;
    return elementAddress.halfwordPointer;
}

/* Builds g_TeamLogoCanvas and its CLUT from one sample character and one sample background. */
void ComposeSampleTeamLogo(s32 character, s32 background)
{
    s32 index;
    u16 *clutDst0;
    u16 *clutDst1;
    u16 *dst;
    u16 *src;
    u16 *src0;
    s32 row0;
    s32 row1;
    s32 outer;
    s32 j;
    u16 value;
    u16 fill;

    row1 = background / 2 + 10;
    background &= 1;
    index = 1;
    clutDst0 = g_TeamLogoSwatches;
    row0 = character / 2;
    character = TeamLogoParity(character);
    src = TeamLogoPaletteAddress(g_TeamLogoSampleData, row0, character) + 1;

    do {
        *clutDst0++ = *src++;
        index++;
    } while (index < 12);

    if (index < 16) {
        clutDst1 = &g_TeamLogoClut[index];
        src = TeamLogoClutAddress(
            g_TeamLogoSampleData, row1, background, index);

        do {
            *clutDst1++ = *src++;
            index++;
        } while (index < 16);
    }

    dst = g_TeamLogoCanvas.halfwords;
    outer = 0;
    src0 = &g_TeamLogoSampleData[row0].canvas[0][0];
    src = &g_TeamLogoSampleData[row1].canvas[0][0];

    for (; outer < 64; outer++) {
        for (j = 0; j < 16; j++) {
            value = *src0;

            if ((value & 0x000F) == 0) {
                fill = *src & 0x000F;
                value |= fill;
            }
            if ((value & 0x00F0) == 0) {
                fill = *src & 0x00F0;
                value |= fill;
            }
            if ((value & 0x0F00) == 0) {
                fill = *src & 0x0F00;
                value |= fill;
            }
            if ((value & 0xF000) == 0) {
                fill = *src & 0xF000;
                value |= fill;
            }
            src0++;
            *dst++ = value;
            src++;
        }
    }
}
