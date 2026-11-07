#ifndef GAME_PRIZE_MONEY_H
#define GAME_PRIZE_MONEY_H

#include "common.h"

typedef enum GrandPrixPrizePlace {
    PRIZE_PLACE_FIRST,
    PRIZE_PLACE_SECOND,
    PRIZE_PLACE_THIRD,
    PRIZE_PLACE_COUNT
} GrandPrixPrizePlace;

enum {
    GRAND_PRIX_PRIZE_CLASS_COUNT = 6,
    RACE_MAX_PRIZE_MONEY = 999999999,
};

s32 ClampPrizeMoney(s32 money);
s32 CreditPrizeMoney(s32 balance, s32 amount);

typedef struct GrandPrixPrizeTable {
    s32 values[4][GRAND_PRIX_PRIZE_CLASS_COUNT][PRIZE_PLACE_COUNT];
} GrandPrixPrizeTable;

typedef struct RagePrizeMoneyRawStorage {
    unsigned char prefix[8];
    unsigned char values[280];
} RagePrizeMoneyRawStorage;

typedef union RagePrizeMoneyStorage {
    RagePrizeMoneyRawStorage raw;
    GrandPrixPrizeTable prizes;
} RagePrizeMoneyStorage;

_Static_assert(sizeof(RagePrizeMoneyStorage) == sizeof(GrandPrixPrizeTable),
               "raw and typed prize tables must cover the same bytes");

extern RagePrizeMoneyStorage g_PrizeMoneyState;
#define g_PrizeMoney (g_PrizeMoneyState.prizes)

#endif
