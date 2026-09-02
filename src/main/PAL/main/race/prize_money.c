#include "game/prize_money.h"

s32 ClampPrizeMoney(s32 money) {
    return money > RACE_MAX_PRIZE_MONEY ? RACE_MAX_PRIZE_MONEY : money;
}

s32 CreditPrizeMoney(s32 balance, s32 amount) {
    balance = ClampPrizeMoney(balance);
    if (amount <= 0 || balance >= RACE_MAX_PRIZE_MONEY) {
        return balance;
    }
    if (amount > RACE_MAX_PRIZE_MONEY - balance) {
        return RACE_MAX_PRIZE_MONEY;
    }
    return balance + amount;
}
