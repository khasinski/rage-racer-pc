#include <stdio.h>

#include "game/menu.h"
#include "game/race.h"
#include "game/render.h"

enum {
    MONEY_TEXT_CAPACITY = 16,
    MONEY_LABEL_X = 0x10,
    MONEY_VALUE_X = 0x12,
    PRIZE_LABEL_Y = 128,
    PRIZE_VALUE_Y = 140,
    TOTAL_LABEL_Y = 160,
    TOTAL_VALUE_Y = 172,
    BONUS_LABEL_Y = 192,
    BONUS_VALUE_Y = 204,
    MONEY_TEXT_CLUT = 0x7812,
};

static void DrawMoneyRow(s32 yOffset, s32 labelY, s32 valueY,
                         const char *label, s32 amount) {
    char moneyText[MONEY_TEXT_CAPACITY];

    DrawProportionalText(MONEY_LABEL_X, yOffset + labelY, label,
                         MONEY_TEXT_CLUT);
    snprintf(moneyText, sizeof(moneyText), g_FmtMoney, amount);
    DrawProportionalText(MONEY_VALUE_X, yOffset + valueY, moneyText,
                         MONEY_TEXT_CLUT);
}

void DrawPrizeMoneyPanel(s32 yOffset) {
    DrawMoneyRow(yOffset, PRIZE_LABEL_Y, PRIZE_VALUE_Y,
                 g_CaptionPrizeMoney, g_PrizeAmount);
    DrawMoneyRow(yOffset, TOTAL_LABEL_Y, TOTAL_VALUE_Y,
                 g_CaptionTotalMoney, g_RaceProgress->money);

    if (g_ClassPromoted) {
        DrawMoneyRow(yOffset, BONUS_LABEL_Y, BONUS_VALUE_Y,
                     g_CaptionPromotionBonus, g_PromotionBonus);
    }
}
