#include "common.h"
#include "game/memcard.h"

void CardSeekParam(s32 param) {
    ClearMemoryCardHwEvents();
    _card_info((u8)param);
    WaitMemoryCardHwEvent();
}


s32 CardReadStatusPair(s32 high, s32 low) {
    s32 cmd;
    s32 ret;
    s32 event;

    cmd = (high * 16) + low;

    _card_info(cmd);
    event = WaitMemoryCardHwEvent();

    switch (event) {
    case 1:
        ret = 1;
        break;
    case 2:
        ret = -3;
        break;
    case 3:
        ret = -1;
        break;
    case 4:
        ret = 2;
        ClearMemoryCardSwEvents();
        _card_clear(cmd);
        WaitMemoryCardSwEvent();
        break;
    default:
        ret = -3;
        break;
    }

    if (ret == -1 || ret == -3) {
        return ret;
    }

    ClearMemoryCardHwEvents();
    _card_load(cmd);
    event = WaitMemoryCardHwEvent();

    switch (event) {
    case 1:
        return ret;
    case 2:
        ret = -3;
        break;
    case 3:
        ret = -1;
        break;
    case 4:
        ret = -2;
        break;
    default:
        ret = -3;
        break;
    }

    return ret;
}
