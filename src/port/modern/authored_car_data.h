#ifndef RAGE_AUTHORED_CAR_DATA_H
#define RAGE_AUTHORED_CAR_DATA_H

#include <stddef.h>
#include <stdint.h>
#include "render/render_world.h"
#include "erriso_body.inc"
#include "erriso_rival.inc"
#include "abeille_body.inc"
#include "abeille_rival.inc"
#include "pegase_body.inc"
#include "pegase_rival.inc"

typedef struct AuthoredCarMaterial {
    uint16_t source, page, clut, cacheSlot;
} AuthoredCarMaterial;

typedef struct AuthoredCarReplacement {
    const char *name;
    uint16_t assetKey, assetSet, submesh;
    const unsigned char *bytes;
    size_t byteCount;
    const AuthoredCarMaterial *materials;
    size_t materialCount;
} AuthoredCarReplacement;

/* Source slots belong to the authored OBJ; cache slots belong to each
 * extracted bank. Live imports resolve the same texture identity directly. */
static const AuthoredCarMaterial s_errisoPlayerMaterials[] = {
    {0, 10, 0x3baf, 0},
    {1, 10, 0x3bef, 1},
    {2, 10, 0x7801, 2},
    {3, 11, 0x382f, 3},
    {4, 11, 0x386f, 4},
    {5, 11, 0x38af, 5},
    {6, 11, 0x39af, 6},
    {7, 11, 0x39ef, 7},
    {8, 11, 0x3a2f, 8},
    {9, 11, 0x3b2f, 9},
};
static const AuthoredCarMaterial s_errisoRivalMaterials[] = {
    {0, 10, 0x7802, 0},
    {13, 12, 0x78c7, 13},
    {14, 12, 0x78c8, 14},
    {19, 13, 0x78c9, 19},
};
static const AuthoredCarMaterial s_abeillePlayerMaterials[] = {
    {0, 10, 0x3baf, 0},
    {1, 10, 0x3bef, 1},
    {2, 10, 0x7801, 2},
    {3, 11, 0x382f, 3},
    {4, 11, 0x386f, 4},
    {5, 11, 0x38ef, 5},
    {6, 11, 0x39af, 6},
    {7, 11, 0x39ef, 7},
    {8, 11, 0x3a2f, 8},
    {9, 11, 0x3a6f, 9},
    {10, 11, 0x3b2f, 10},
    {11, 11, 0x3c2f, 11},
};
static const AuthoredCarMaterial s_abeilleRivalMaterials[] = {
    {0, 10, 0x7802, 0},
    {11, 12, 0x78c7, 11},
    {12, 12, 0x78c8, 12},
    {17, 13, 0x78c9, 17},
};
static const AuthoredCarMaterial s_pegasePlayerMaterials[] = {
    {0, 10, 0x3baf, 0},
    {1, 10, 0x3bef, 1},
    {2, 10, 0x7801, 2},
    {3, 11, 0x382f, 3},
    {4, 11, 0x38ef, 4},
    {5, 11, 0x396f, 5},
    {6, 11, 0x39af, 6},
    {7, 11, 0x39ef, 7},
    {8, 11, 0x3a2f, 8},
    {9, 11, 0x3a6f, 9},
    {10, 11, 0x3aaf, 10},
    {11, 11, 0x3aef, 11},
    {12, 11, 0x3b2f, 12},
    {13, 11, 0x3b6f, 13},
};
static const AuthoredCarMaterial s_pegaseRivalMaterials[] = {
    {0, 10, 0x7802, 0},
    {13, 12, 0x78c7, 13},
    {14, 12, 0x78c8, 14},
    {15, 12, 0x78ca, 15},
    {20, 13, 0x78c9, 20},
};
static const AuthoredCarMaterial s_pegaseRivalAlternateMaterials[] = {
    {0, 10, 0x7802, 0},
    {13, 12, 0x78c7, 12},
    {14, 12, 0x78c8, 13},
    {15, 12, 0x78ca, 14},
    {20, 13, 0x78c9, 21},
};

#define AUTHORED_CAR(name, key, set, part, data, map) \
    {name, key, set, part, data, sizeof(data), map, sizeof(map)/sizeof(map[0])}
static const AuthoredCarReplacement s_authoredCars[] = {
    AUTHORED_CAR("Erriso", 10, RAGE_RENDER_ASSET_MODEL_BANK, 0, s_errisoBody, s_errisoPlayerMaterials),
    AUTHORED_CAR("Erriso", 96, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 10, s_errisoRivalBody, s_errisoRivalMaterials),
    AUTHORED_CAR("Erriso", 98, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 10, s_errisoRivalBody, s_errisoRivalMaterials),
    AUTHORED_CAR("Erriso", 100, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 10, s_errisoRivalBody, s_errisoRivalMaterials),
    AUTHORED_CAR("Abeille", 18, RAGE_RENDER_ASSET_MODEL_BANK, 0, s_abeille_body, s_abeillePlayerMaterials),
    AUTHORED_CAR("Abeille", 102, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 10, s_abeille_rival, s_abeilleRivalMaterials),
    AUTHORED_CAR("Abeille", 104, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 10, s_abeille_rival, s_abeilleRivalMaterials),
    AUTHORED_CAR("Abeille", 106, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 10, s_abeille_rival, s_abeilleRivalMaterials),
    AUTHORED_CAR("Abeille", 108, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 10, s_abeille_rival, s_abeilleRivalMaterials),
    AUTHORED_CAR("Abeille", 110, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 10, s_abeille_rival, s_abeilleRivalMaterials),
    AUTHORED_CAR("Pegase", 24, RAGE_RENDER_ASSET_MODEL_BANK, 0, s_pegase_body, s_pegasePlayerMaterials),
    AUTHORED_CAR("Pegase", 112, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 10, s_pegase_rival, s_pegaseRivalMaterials),
    AUTHORED_CAR("Pegase", 114, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 10, s_pegase_rival, s_pegaseRivalMaterials),
    AUTHORED_CAR("Pegase", 116, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 10, s_pegase_rival, s_pegaseRivalMaterials),
    AUTHORED_CAR("Pegase", 118, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 10, s_pegase_rival, s_pegaseRivalMaterials),
    AUTHORED_CAR("Pegase", 94, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 10, s_pegase_rival, s_pegaseRivalAlternateMaterials),
    AUTHORED_CAR("Pegase", 120, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 10, s_pegase_rival, s_pegaseRivalAlternateMaterials),
    AUTHORED_CAR("Pegase", 122, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 10, s_pegase_rival, s_pegaseRivalAlternateMaterials),
    AUTHORED_CAR("Pegase", 124, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 10, s_pegase_rival, s_pegaseRivalAlternateMaterials),
    AUTHORED_CAR("Pegase", 126, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 10, s_pegase_rival, s_pegaseRivalAlternateMaterials),
};
#undef AUTHORED_CAR
#define RAGE_AUTHORED_CAR_COUNT (sizeof(s_authoredCars)/sizeof(s_authoredCars[0]))

#endif

