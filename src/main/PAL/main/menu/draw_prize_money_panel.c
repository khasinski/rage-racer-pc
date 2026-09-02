#include <stdio.h>

#include "game/menu.h"
#include "game/race.h"
#include "game/render.h"

void DrawPrizeMoneyPanel(s32 yOffset) {
    char moneyText[16];

    DrawProportionalText(0x10, yOffset + 128, g_CaptionPrizeMoney, 0x7812);
    snprintf(moneyText, sizeof(moneyText), g_FmtMoney, g_PrizeAmount);
    DrawProportionalText(0x12, yOffset + 140, moneyText, 0x7812);
    DrawProportionalText(0x10, yOffset + 160, g_CaptionTotalMoney, 0x7812);
    snprintf(moneyText, sizeof(moneyText), g_FmtMoney,
             g_RaceProgress->money.value);
    DrawProportionalText(0x12, yOffset + 172, moneyText, 0x7812);

    if (g_ClassPromoted) {
        DrawProportionalText(0x10, yOffset + 192, g_CaptionPromotionBonus,
                             0x7812);
        snprintf(moneyText, sizeof(moneyText), g_FmtMoney, g_PromotionBonus);
        DrawProportionalText(0x12, yOffset + 204, moneyText, 0x7812);
    }
}
