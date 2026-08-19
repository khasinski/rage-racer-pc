#include "game/menu_controller.h"

static s32 WrapCursor(s32 selection, s32 itemCount) {
    selection %= itemCount;
    return selection < 0 ? selection + itemCount : selection;
}

MenuCursorResult MenuCursorMove(s32 selection, s32 itemCount,
                                s32 direction, u32 enabledMask) {
    MenuCursorResult result;
    s32 current;
    s32 remaining;

    result.selection = selection;
    result.moved = 0;
    if (itemCount <= 0 || direction == 0) return result;

    direction = direction < 0 ? -1 : 1;
    current = WrapCursor(selection, itemCount);
    for (remaining = itemCount; remaining > 0; remaining--) {
        current = WrapCursor(current + direction, itemCount);
        if (enabledMask == 0 || (enabledMask & (1u << current)) != 0) {
            result.selection = current;
            result.moved = current != selection;
            return result;
        }
    }
    return result;
}

MenuAction MenuResolveAction(u16 pressed, u16 confirmMask, u16 cancelMask) {
    if ((pressed & confirmMask) != 0) return MENU_ACTION_CONFIRM;
    if ((pressed & cancelMask) != 0) return MENU_ACTION_CANCEL;
    return MENU_ACTION_NONE;
}

MenuSessionCommands MenuSessionStep(MenuSession *session, s32 direction,
                                    u16 pressed) {
    MenuCursorResult cursor = MenuCursorMove(
        session->selection, session->itemCount, direction,
        session->enabledMask);
    MenuSessionCommands commands;
    session->selection = cursor.selection;
    commands.moved = cursor.moved;
    commands.moveCount = cursor.moved;
    commands.action = MenuResolveAction(pressed, PAD_CONFIRM, PAD_CANCEL);
    return commands;
}

MenuSessionCommands MenuSessionStepVertical(MenuSession *session,
                                            u16 pressed) {
    MenuSessionCommands commands;
    MenuCursorResult cursor;

    commands.moveCount = 0;
    if ((pressed & PAD_UP) != 0) {
        cursor = MenuCursorMove(session->selection, session->itemCount, -1,
                                session->enabledMask);
        session->selection = cursor.selection;
        commands.moveCount += cursor.moved;
    }
    if ((pressed & PAD_DOWN) != 0) {
        cursor = MenuCursorMove(session->selection, session->itemCount, 1,
                                session->enabledMask);
        session->selection = cursor.selection;
        commands.moveCount += cursor.moved;
    }
    commands.moved = commands.moveCount != 0;
    commands.action = MenuResolveAction(pressed, PAD_CONFIRM, PAD_CANCEL);
    return commands;
}

s32 MenuViewIsSettled(s32 current, s32 target, s32 tolerance) {
    s32 distance;

    if (tolerance < 0) return 0;
    distance = current >= target ? current - target : target - current;
    return distance <= tolerance;
}

s32 MenuExitIsReady(s32 outgoingProgress, s32 viewOffset,
                    s32 minimumViewOffset) {
    return outgoingProgress <= 0 && viewOffset > minimumViewOffset;
}
