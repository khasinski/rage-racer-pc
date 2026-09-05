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
#include "esperanza_body.inc"
#include "esperanza_rival.inc"
#include "esperanza_rival_late.inc"
#include "esperanza_rival_duplicate.inc"
#include "esperanza_rival_slot1.inc"
#include "esperanza_rival_slot2.inc"
#include "acceron_body.inc"
#include "acceron_rival.inc"
#include "bayonet_body.inc"
#include "bayonet_rival.inc"
#include "hijack_body.inc"
#include "hijack_rival.inc"
#include "hijack_rival_alternate.inc"
#include "fatalita_body.inc"
#include "fatalita_rival.inc"
#include "istante_body.inc"
#include "istante_rival.inc"
#include "ghepardo_body.inc"
#include "ghepardo_rival.inc"
#include "vainqure_body.inc"
#include "vainqure_rival.inc"
#include "bulshade_body.inc"
#include "bulshade_rival.inc"
#include "bulshade_rival_alternate.inc"
#include "squaldon_body.inc"
#include "squaldon_rival.inc"
#include "compacta_rival_early.inc"
#include "compactb_rival_early.inc"
#include "compactc_rival_early.inc"

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

static const AuthoredCarMaterial s_esperanzaPlayerMaterials[] = {
    {1, 10, 0x3bef, 1},
    {2, 10, 0x7801, 2},
    {3, 11, 0x382f, 3},
    {4, 11, 0x386f, 4},
    {5, 11, 0x38ef, 5},
    {6, 11, 0x39af, 6},
    {7, 11, 0x39ef, 7},
    {8, 11, 0x3a2f, 8},
    {9, 11, 0x3a6f, 9},
    {10, 11, 0x3aaf, 10},
    {11, 11, 0x3aef, 11},
    {12, 11, 0x3b2f, 12},
};
static const AuthoredCarMaterial s_esperanzaDuplicate88Materials[] = {
    {0,10,0x7802,0}, {12,12,0x7907,12}, {13,12,0x7908,13}, {17,13,0x7909,17},
};
static const AuthoredCarMaterial s_esperanzaDuplicate96Materials[] = {
    {0,10,0x7802,0}, {12,12,0x7907,15}, {13,12,0x7908,16}, {17,13,0x7909,20},
};
static const AuthoredCarMaterial s_esperanzaSlot1Materials[] = {
    {0,10,0x7802,0}, {8,12,0x7887,8}, {9,12,0x7888,9}, {15,13,0x7889,15},
};
static const AuthoredCarMaterial s_esperanzaSlot2Materials[] = {
    {0,10,0x7802,0}, {10,12,0x78c7,10}, {11,12,0x78c8,11}, {16,13,0x78c9,16},
};
static const AuthoredCarMaterial s_esperanzaBank88Materials[] = {
    {0, 10, 0x7802, 0},
    {6, 12, 0x7847, 6},
    {7, 12, 0x7848, 7},
    {14, 13, 0x7849, 14},
};
static const AuthoredCarMaterial s_esperanzaBank94Materials[] = {
    {0, 10, 0x7802, 0},
    {7, 12, 0x7847, 7},
    {8, 12, 0x7848, 8},
    {15, 13, 0x7849, 19},
};
static const AuthoredCarMaterial s_esperanzaBank96Materials[] = {
    {0, 10, 0x7802, 0},
    {6, 12, 0x7847, 8},
    {7, 12, 0x7848, 9},
    {14, 13, 0x7849, 17},
};
static const AuthoredCarMaterial s_esperanzaBank102Materials[] = {
    {0, 10, 0x7802, 0},
    {7, 12, 0x7847, 7},
    {8, 12, 0x7848, 8},
    {15, 13, 0x7849, 15},
};
static const AuthoredCarMaterial s_esperanzaBank112Materials[] = {
    {0, 10, 0x7802, 0},
    {7, 12, 0x7847, 8},
    {8, 12, 0x7848, 9},
    {15, 13, 0x7849, 18},
};

/* Acceron's base player uses the same texture identities and cached slots
 * as Esperanza; each bank still supplies its own images. */
static const AuthoredCarMaterial s_acceronRivalMaterials[] = {
    {0, 10, 0x7802, 0},
    {10, 12, 0x7887, 10},
    {11, 12, 0x7888, 11},
    {12, 12, 0x788a, 12},
    {18, 13, 0x7889, 18},
};

static const AuthoredCarMaterial s_bayonetRivalMaterials[] = {
    {0, 10, 0x7802, 0},
    {9, 12, 0x7887, 9},
    {10, 12, 0x7888, 10},
    {16, 13, 0x7889, 16},
};

static const AuthoredCarMaterial s_hijackRivalMaterials[] = {
    {0, 10, 0x7802, 0}, {10, 12, 0x7887, 10},
    {11, 12, 0x7888, 11}, {12, 12, 0x788a, 12}, {19, 13, 0x7889, 19},
};
static const AuthoredCarMaterial s_hijackRivalAlternateMaterials[] = {
    {0, 10, 0x7802, 0}, {9, 12, 0x7887, 9},
    {10, 12, 0x7888, 10}, {11, 12, 0x788a, 11}, {20, 13, 0x7889, 20},
};

static const AuthoredCarMaterial s_fatalitaRivalMaterials[] = {
    {0, 10, 0x7802, 0}, {13, 12, 0x7907, 13},
    {14, 12, 0x7908, 14}, {18, 13, 0x7909, 18},
};

static const AuthoredCarMaterial s_istantePlayerMaterials[] = {
    {1, 10, 0x3bef, 1}, {2, 10, 0x7801, 2},
    {3, 11, 0x382f, 3}, {4, 11, 0x386f, 4}, {5, 11, 0x39af, 5},
    {6, 11, 0x39ef, 6}, {7, 11, 0x3a2f, 7}, {8, 11, 0x3a6f, 8},
    {9, 11, 0x3aaf, 9}, {10, 11, 0x3aef, 10},
    {11, 11, 0x3b2f, 11}, {12, 11, 0x3bef, 12},
};
static const AuthoredCarMaterial s_istanteRivalMaterials[] = {
    {0, 10, 0x7802, 0}, {16, 12, 0x7907, 16},
    {17, 12, 0x7908, 17}, {21, 13, 0x7909, 21},
};

static const AuthoredCarMaterial s_ghepardoPlayerMaterials[] = {
    {1, 10, 0x3bef, 1}, {2, 10, 0x7801, 2},
    {3, 11, 0x382f, 3}, {4, 11, 0x386f, 4}, {5, 11, 0x38af, 5},
    {6, 11, 0x39af, 6}, {7, 11, 0x39ef, 7}, {8, 11, 0x3a2f, 8},
    {9, 11, 0x3a6f, 9}, {10, 11, 0x3aaf, 10},
    {11, 11, 0x3aef, 11}, {12, 11, 0x3b2f, 12},
};
static const AuthoredCarMaterial s_ghepardoRivalMaterials[] = {
    {0, 10, 0x7802, 0}, {15, 12, 0x7907, 15},
    {16, 12, 0x7908, 16}, {17, 12, 0x7909, 17},
    {18, 12, 0x790a, 18}, {22, 13, 0x7909, 22},
};

static const AuthoredCarMaterial s_vainqurePlayerMaterials[] = {
    {1, 11, 0x382f, 1}, {2, 11, 0x386f, 2}, {3, 11, 0x38af, 3},
    {4, 11, 0x38ef, 4}, {5, 11, 0x392f, 5}, {6, 11, 0x396f, 6},
    {7, 11, 0x39af, 7}, {8, 11, 0x39ef, 8},
};
static const AuthoredCarMaterial s_vainqureRivalMaterials[] = {
    {3, 12, 0x7840, 3}, {4, 12, 0x7841, 4}, {5, 12, 0x7842, 5},
    {6, 12, 0x7843, 6}, {7, 12, 0x7844, 7},
};

static const AuthoredCarMaterial s_bulshadePlayerMaterials[] = {
    {1,11,0x382f,1}, {2,11,0x386f,2}, {3,11,0x38af,3},
    {4,11,0x38ef,4}, {5,11,0x392f,5}, {6,11,0x396f,6},
    {7,11,0x39af,7}, {8,11,0x39ef,8}, {9,11,0x3a2f,9}, {10,11,0x3a6f,10},
};
static const AuthoredCarMaterial s_bulshadeRivalMaterials[] = {
    {26,14,0x7880,26}, {27,14,0x7881,27}, {28,14,0x7882,28},
    {29,14,0x7883,29}, {30,14,0x7884,30}, {31,14,0x7885,31},
    {32,14,0x7886,32}, {33,14,0x7888,33}, {34,14,0x7889,34},
};
static const AuthoredCarMaterial s_bulshadeRivalAlternateMaterials[] = {
    {15,13,0x78c2,15}, {16,13,0x78c3,16},
    {35,14,0x78c0,35}, {36,14,0x78c1,36}, {37,14,0x78c2,37},
    {38,14,0x78c4,38}, {39,14,0x78c5,39}, {40,14,0x78c6,40},
    {41,14,0x78c7,41}, {42,14,0x78c8,42},
};

static const AuthoredCarMaterial s_squaldonPlayerMaterials[] = {
    {1, 11, 0x382f, 1},
    {2, 11, 0x386f, 2},
    {3, 11, 0x38af, 3},
    {4, 11, 0x38ef, 4},
    {5, 11, 0x392f, 5},
    {6, 11, 0x396f, 6},
    {7, 11, 0x39af, 7},
    {8, 11, 0x39ef, 8},
    {9, 11, 0x3a2f, 9},
    {10, 11, 0x3a6f, 10},
    {11, 11, 0x3aaf, 11},
    {12, 11, 0x3aef, 12},
    {13, 11, 0x3b2f, 13},
    {14, 11, 0x3b6f, 14},
};
static const AuthoredCarMaterial s_squaldonRivalMaterials[] = {
    {8, 12, 0x7900, 8},
    {9, 12, 0x7901, 9},
    {10, 12, 0x7902, 10},
    {11, 12, 0x7903, 11},
    {12, 12, 0x7904, 12},
    {13, 12, 0x7905, 13},
    {14, 12, 0x7906, 14},
    {19, 13, 0x7907, 19},
    {20, 13, 0x7908, 20},
};

static const AuthoredCarMaterial s_compactaEarly88Materials[] = {
    {3,10,0x7900,3},
    {23,14,0x7840,23},
    {26,14,0x7880,26},
    {29,14,0x78c0,29},
};
static const AuthoredCarMaterial s_compactaEarly96Materials[] = {
    {3,10,0x7900,5},
    {23,14,0x7840,26},
    {26,14,0x7880,29},
    {29,14,0x78c0,32},
};
static const AuthoredCarMaterial s_compactbEarly88Materials[] = {
    {4,10,0x7902,4},
    {24,14,0x7842,24},
    {27,14,0x7882,27},
    {30,14,0x78c2,30},
};
static const AuthoredCarMaterial s_compactbEarly96Materials[] = {
    {4,10,0x7902,6},
    {24,14,0x7842,27},
    {27,14,0x7882,30},
    {30,14,0x78c2,33},
};
static const AuthoredCarMaterial s_compactcEarly88Materials[] = {
    {3,10,0x7900,3},
    {25,14,0x7844,25},
    {28,14,0x7884,28},
    {31,14,0x78c4,31},
};
static const AuthoredCarMaterial s_compactcEarly96Materials[] = {
    {3,10,0x7900,5},
    {25,14,0x7844,28},
    {28,14,0x7884,31},
    {31,14,0x78c4,34},
};

#define AUTHORED_CAR(name, key, set, part, data, map) \
    {name, key, set, part, data, sizeof(data), map, sizeof(map)/sizeof(map[0])}
static const AuthoredCarReplacement s_authoredCars[] = {
    AUTHORED_CAR("Compact A", 88, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 20, s_compacta_rival_early, s_compactaEarly88Materials),
    AUTHORED_CAR("Compact A", 90, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 20, s_compacta_rival_early, s_compactaEarly88Materials),
    AUTHORED_CAR("Compact A", 92, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 20, s_compacta_rival_early, s_compactaEarly88Materials),
    AUTHORED_CAR("Compact A", 96, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 20, s_compacta_rival_early, s_compactaEarly96Materials),
    AUTHORED_CAR("Compact A", 98, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 20, s_compacta_rival_early, s_compactaEarly96Materials),
    AUTHORED_CAR("Compact A", 100, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 20, s_compacta_rival_early, s_compactaEarly96Materials),
    AUTHORED_CAR("Compact B", 88, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 25, s_compactb_rival_early, s_compactbEarly88Materials),
    AUTHORED_CAR("Compact B", 90, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 25, s_compactb_rival_early, s_compactbEarly88Materials),
    AUTHORED_CAR("Compact B", 92, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 25, s_compactb_rival_early, s_compactbEarly88Materials),
    AUTHORED_CAR("Compact B", 96, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 25, s_compactb_rival_early, s_compactbEarly96Materials),
    AUTHORED_CAR("Compact B", 98, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 25, s_compactb_rival_early, s_compactbEarly96Materials),
    AUTHORED_CAR("Compact B", 100, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 25, s_compactb_rival_early, s_compactbEarly96Materials),
    AUTHORED_CAR("Compact C", 88, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 30, s_compactc_rival_early, s_compactcEarly88Materials),
    AUTHORED_CAR("Compact C", 90, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 30, s_compactc_rival_early, s_compactcEarly88Materials),
    AUTHORED_CAR("Compact C", 92, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 30, s_compactc_rival_early, s_compactcEarly88Materials),
    AUTHORED_CAR("Compact C", 96, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 30, s_compactc_rival_early, s_compactcEarly96Materials),
    AUTHORED_CAR("Compact C", 98, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 30, s_compactc_rival_early, s_compactcEarly96Materials),
    AUTHORED_CAR("Compact C", 100, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 30, s_compactc_rival_early, s_compactcEarly96Materials),
    AUTHORED_CAR("Esperanza", 88, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 5, s_esperanza_rival_slot1, s_esperanzaSlot1Materials),
    AUTHORED_CAR("Esperanza", 90, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 5, s_esperanza_rival_slot1, s_esperanzaSlot1Materials),
    AUTHORED_CAR("Esperanza", 92, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 5, s_esperanza_rival_slot1, s_esperanzaSlot1Materials),
    AUTHORED_CAR("Esperanza", 88, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 10, s_esperanza_rival_slot2, s_esperanzaSlot2Materials),
    AUTHORED_CAR("Esperanza", 90, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 10, s_esperanza_rival_slot2, s_esperanzaSlot2Materials),
    AUTHORED_CAR("Esperanza", 92, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 10, s_esperanza_rival_slot2, s_esperanzaSlot2Materials),
    AUTHORED_CAR("Esperanza", 88, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 15, s_esperanza_rival_duplicate, s_esperanzaDuplicate88Materials),
    AUTHORED_CAR("Esperanza", 90, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 15, s_esperanza_rival_duplicate, s_esperanzaDuplicate88Materials),
    AUTHORED_CAR("Esperanza", 92, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 15, s_esperanza_rival_duplicate, s_esperanzaDuplicate88Materials),
    AUTHORED_CAR("Esperanza", 96, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 15, s_esperanza_rival_duplicate, s_esperanzaDuplicate96Materials),
    AUTHORED_CAR("Esperanza", 98, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 15, s_esperanza_rival_duplicate, s_esperanzaDuplicate96Materials),
    AUTHORED_CAR("Esperanza", 100, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 15, s_esperanza_rival_duplicate, s_esperanzaDuplicate96Materials),
    AUTHORED_CAR("Squaldon", 72, RAGE_RENDER_ASSET_MODEL_BANK, 0, s_squaldon_body, s_squaldonPlayerMaterials),
    AUTHORED_CAR("Squaldon", 128, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 15, s_squaldon_rival, s_squaldonRivalMaterials),
    AUTHORED_CAR("Squaldon", 130, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 15, s_squaldon_rival, s_squaldonRivalMaterials),
    AUTHORED_CAR("Squaldon", 132, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 15, s_squaldon_rival, s_squaldonRivalMaterials),
    AUTHORED_CAR("Squaldon", 134, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 15, s_squaldon_rival, s_squaldonRivalMaterials),
    AUTHORED_CAR("Bulshade", 70, RAGE_RENDER_ASSET_MODEL_BANK, 0, s_bulshade_body, s_bulshadePlayerMaterials),
    AUTHORED_CAR("Bulshade", 128, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 0, s_bulshade_rival, s_bulshadeRivalMaterials),
    AUTHORED_CAR("Bulshade", 128, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 5, s_bulshade_rival_alternate, s_bulshadeRivalAlternateMaterials),
    AUTHORED_CAR("Bulshade", 130, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 0, s_bulshade_rival, s_bulshadeRivalMaterials),
    AUTHORED_CAR("Bulshade", 130, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 5, s_bulshade_rival_alternate, s_bulshadeRivalAlternateMaterials),
    AUTHORED_CAR("Bulshade", 132, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 0, s_bulshade_rival, s_bulshadeRivalMaterials),
    AUTHORED_CAR("Bulshade", 132, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 5, s_bulshade_rival_alternate, s_bulshadeRivalAlternateMaterials),
    AUTHORED_CAR("Bulshade", 134, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 0, s_bulshade_rival, s_bulshadeRivalMaterials),
    AUTHORED_CAR("Bulshade", 134, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 5, s_bulshade_rival_alternate, s_bulshadeRivalAlternateMaterials),
    AUTHORED_CAR("Vainqure", 68, RAGE_RENDER_ASSET_MODEL_BANK, 0, s_vainqure_body, s_vainqurePlayerMaterials),
    AUTHORED_CAR("Vainqure", 128, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 10, s_vainqure_rival, s_vainqureRivalMaterials),
    AUTHORED_CAR("Vainqure", 130, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 10, s_vainqure_rival, s_vainqureRivalMaterials),
    AUTHORED_CAR("Vainqure", 132, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 10, s_vainqure_rival, s_vainqureRivalMaterials),
    AUTHORED_CAR("Vainqure", 134, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 10, s_vainqure_rival, s_vainqureRivalMaterials),
    AUTHORED_CAR("Ghepardo", 66, RAGE_RENDER_ASSET_MODEL_BANK, 0, s_ghepardo_body, s_ghepardoPlayerMaterials),
    AUTHORED_CAR("Ghepardo", 94, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 15, s_ghepardo_rival, s_ghepardoRivalMaterials),
    AUTHORED_CAR("Ghepardo", 120, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 15, s_ghepardo_rival, s_ghepardoRivalMaterials),
    AUTHORED_CAR("Ghepardo", 122, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 15, s_ghepardo_rival, s_ghepardoRivalMaterials),
    AUTHORED_CAR("Ghepardo", 124, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 15, s_ghepardo_rival, s_ghepardoRivalMaterials),
    AUTHORED_CAR("Ghepardo", 126, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 15, s_ghepardo_rival, s_ghepardoRivalMaterials),
    AUTHORED_CAR("Istante", 62, RAGE_RENDER_ASSET_MODEL_BANK, 0, s_istante_body, s_istantePlayerMaterials),
    AUTHORED_CAR("Istante", 112, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 15, s_istante_rival, s_istanteRivalMaterials),
    AUTHORED_CAR("Istante", 114, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 15, s_istante_rival, s_istanteRivalMaterials),
    AUTHORED_CAR("Istante", 116, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 15, s_istante_rival, s_istanteRivalMaterials),
    AUTHORED_CAR("Istante", 118, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 15, s_istante_rival, s_istanteRivalMaterials),
    AUTHORED_CAR("Fatalita", 56, RAGE_RENDER_ASSET_MODEL_BANK, 0, s_fatalita_body, s_esperanzaPlayerMaterials),
    AUTHORED_CAR("Fatalita", 102, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 15, s_fatalita_rival, s_fatalitaRivalMaterials),
    AUTHORED_CAR("Fatalita", 104, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 15, s_fatalita_rival, s_fatalitaRivalMaterials),
    AUTHORED_CAR("Fatalita", 106, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 15, s_fatalita_rival, s_fatalitaRivalMaterials),
    AUTHORED_CAR("Fatalita", 108, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 15, s_fatalita_rival, s_fatalitaRivalMaterials),
    AUTHORED_CAR("Fatalita", 110, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 15, s_fatalita_rival, s_fatalitaRivalMaterials),
    AUTHORED_CAR("Hijack", 52, RAGE_RENDER_ASSET_MODEL_BANK, 0, s_hijack_body, s_esperanzaPlayerMaterials),
    AUTHORED_CAR("Hijack", 94, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 5, s_hijack_rival_alternate, s_hijackRivalAlternateMaterials),
    AUTHORED_CAR("Hijack", 112, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 5, s_hijack_rival, s_hijackRivalMaterials),
    AUTHORED_CAR("Hijack", 114, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 5, s_hijack_rival, s_hijackRivalMaterials),
    AUTHORED_CAR("Hijack", 116, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 5, s_hijack_rival, s_hijackRivalMaterials),
    AUTHORED_CAR("Hijack", 118, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 5, s_hijack_rival, s_hijackRivalMaterials),
    AUTHORED_CAR("Hijack", 120, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 5, s_hijack_rival_alternate, s_hijackRivalAlternateMaterials),
    AUTHORED_CAR("Hijack", 122, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 5, s_hijack_rival_alternate, s_hijackRivalAlternateMaterials),
    AUTHORED_CAR("Hijack", 124, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 5, s_hijack_rival_alternate, s_hijackRivalAlternateMaterials),
    AUTHORED_CAR("Hijack", 126, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 5, s_hijack_rival_alternate, s_hijackRivalAlternateMaterials),
    AUTHORED_CAR("Bayonet", 46, RAGE_RENDER_ASSET_MODEL_BANK, 0, s_bayonet_body, s_esperanzaPlayerMaterials),
    AUTHORED_CAR("Bayonet", 102, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 5, s_bayonet_rival, s_bayonetRivalMaterials),
    AUTHORED_CAR("Bayonet", 104, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 5, s_bayonet_rival, s_bayonetRivalMaterials),
    AUTHORED_CAR("Bayonet", 106, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 5, s_bayonet_rival, s_bayonetRivalMaterials),
    AUTHORED_CAR("Bayonet", 108, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 5, s_bayonet_rival, s_bayonetRivalMaterials),
    AUTHORED_CAR("Bayonet", 110, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 5, s_bayonet_rival, s_bayonetRivalMaterials),
    AUTHORED_CAR("Acceron", 38, RAGE_RENDER_ASSET_MODEL_BANK, 0, s_acceron_body, s_esperanzaPlayerMaterials),
    AUTHORED_CAR("Acceron", 96, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 5, s_acceron_rival, s_acceronRivalMaterials),
    AUTHORED_CAR("Acceron", 98, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 5, s_acceron_rival, s_acceronRivalMaterials),
    AUTHORED_CAR("Acceron", 100, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 5, s_acceron_rival, s_acceronRivalMaterials),
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
    AUTHORED_CAR("Esperanza", 28, RAGE_RENDER_ASSET_MODEL_BANK, 0, s_esperanza_body, s_esperanzaPlayerMaterials),
    AUTHORED_CAR("Esperanza", 88, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 0, s_esperanza_rival, s_esperanzaBank88Materials),
    AUTHORED_CAR("Esperanza", 90, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 0, s_esperanza_rival, s_esperanzaBank88Materials),
    AUTHORED_CAR("Esperanza", 92, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 0, s_esperanza_rival, s_esperanzaBank88Materials),
    AUTHORED_CAR("Esperanza", 94, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 0, s_esperanza_rival_late, s_esperanzaBank94Materials),
    AUTHORED_CAR("Esperanza", 96, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 0, s_esperanza_rival, s_esperanzaBank96Materials),
    AUTHORED_CAR("Esperanza", 98, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 0, s_esperanza_rival, s_esperanzaBank96Materials),
    AUTHORED_CAR("Esperanza", 100, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 0, s_esperanza_rival, s_esperanzaBank96Materials),
    AUTHORED_CAR("Esperanza", 102, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 0, s_esperanza_rival_late, s_esperanzaBank102Materials),
    AUTHORED_CAR("Esperanza", 104, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 0, s_esperanza_rival_late, s_esperanzaBank102Materials),
    AUTHORED_CAR("Esperanza", 106, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 0, s_esperanza_rival_late, s_esperanzaBank102Materials),
    AUTHORED_CAR("Esperanza", 108, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 0, s_esperanza_rival_late, s_esperanzaBank102Materials),
    AUTHORED_CAR("Esperanza", 110, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 0, s_esperanza_rival_late, s_esperanzaBank102Materials),
    AUTHORED_CAR("Esperanza", 112, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 0, s_esperanza_rival_late, s_esperanzaBank112Materials),
    AUTHORED_CAR("Esperanza", 114, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 0, s_esperanza_rival_late, s_esperanzaBank112Materials),
    AUTHORED_CAR("Esperanza", 116, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 0, s_esperanza_rival_late, s_esperanzaBank112Materials),
    AUTHORED_CAR("Esperanza", 118, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 0, s_esperanza_rival_late, s_esperanzaBank112Materials),
    AUTHORED_CAR("Esperanza", 120, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 0, s_esperanza_rival_late, s_esperanzaBank94Materials),
    AUTHORED_CAR("Esperanza", 122, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 0, s_esperanza_rival_late, s_esperanzaBank94Materials),
    AUTHORED_CAR("Esperanza", 124, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 0, s_esperanza_rival_late, s_esperanzaBank94Materials),
    AUTHORED_CAR("Esperanza", 126, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 0, s_esperanza_rival_late, s_esperanzaBank94Materials),
};
#undef AUTHORED_CAR
#define RAGE_AUTHORED_CAR_COUNT (sizeof(s_authoredCars)/sizeof(s_authoredCars[0]))

#endif
