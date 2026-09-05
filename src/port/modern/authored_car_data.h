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

#define AUTHORED_CAR(name, key, set, part, data, map) \
    {name, key, set, part, data, sizeof(data), map, sizeof(map)/sizeof(map[0])}
static const AuthoredCarReplacement s_authoredCars[] = {
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
