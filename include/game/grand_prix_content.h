#ifndef GAME_GRAND_PRIX_CONTENT_H
#define GAME_GRAND_PRIX_CONTENT_H
#include <stddef.h>
#include "common.h"

enum { GRAND_PRIX_CONTENT_CLASS_COUNT = 6 };

typedef struct GrandPrixClassDefinition {
    s32 courseCount;
    s32 record[2];
    s32 nextClass[2];
    s32 unlockRecord[2];
    s32 final[2];
    s32 promotionStream[2];
    s32 assetSeries[2];
    s32 prizeClass;
    s32 promotionBonusIndex;
    s32 freezeScenery;
    s32 coastHighScenery;
    s32 courseOneSpinningScenery;
} GrandPrixClassDefinition;

/* Retail built-in definitions. The Extra finale shares standard record/assets;
 * promotion streams deliberately repeat after class 3. Ending selection is
 * separate and is reached through the series-completion rule. */
static inline const GrandPrixClassDefinition *GrandPrixContentClass(s32 index) {
    static const GrandPrixClassDefinition classes[GRAND_PRIX_CONTENT_CLASS_COUNT] = {
        {3, {0, 6},  {1, 1},   {1, 7},   {0, 0}, {1, 5}, {0, 1}, 0, 0, 0, 0, 0},
        {3, {1, 7},  {2, 2},   {2, 8},   {0, 0}, {2, 6}, {0, 1}, 1, 1, 0, 0, 0},
        {4, {2, 8},  {3, 3},   {3, 9},   {0, 0}, {3, 7}, {0, 1}, 2, 2, 0, 0, 1},
        {4, {3, 9},  {4, 4},   {4, 10},  {0, 0}, {4, 8}, {0, 1}, 3, 3, 0, 0, 1},
        {4, {4, 10}, {-1, 5},  {6, 5},   {1, 0}, {4, 8}, {0, 1}, 4, 4, 0, 1, 1},
        {4, {5, -1}, {-1, -1}, {-1, -1}, {0, 1}, {4, 8}, {0, 0}, 5, -1, 1, 1, 1},
    };
    return (u32)index < GRAND_PRIX_CONTENT_CLASS_COUNT ? &classes[index] : NULL;
}
#endif
