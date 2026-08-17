#include "common.h"
#include "game/audio.h"
#include "game/sound.h"
#include "psyq/snd.h"


s32 StartAudioSlotLoad(s32 slot, u8 *header, u8 *body, u16 *table) {
    s16 vabId;

    if (slot == 3) {
        return (s16)StartVabTransferWithTable(header, body, table);
    }
    if (slot == 1 || slot == 6) {
        return (s16)OpenVabSequenceSlot(slot, header, body, table);
    }

    g_AudioLoadSlot = slot;
    g_SoundScale.vabIds[slot] = SsVabOpenHeadSticky(header, -1, g_VabSpuAddress[slot]);
    /* Reading the slot back is what keeps the opened id in a register: the
       store is a halfword, so the reload is folded into a sign-extend of the
       call result and that value is still there to hand to SsVabTransBody. */
    vabId = g_SoundScale.vabIds[slot];
    if (vabId == -1) {
        printf("%s", g_MsgVabOpenHeadError);
        BiosExit(1);
    }

    g_SoundScale.vabIds[slot] = SsVabTransBody(body, vabId);
    if (g_SoundScale.vabIds[slot] == -1) {
        printf("%s", g_MsgVabTransBodyError);
        BiosExit(1);
    }

    g_VabTransferDone = SsVabTransCompleted(0);
    return g_VabTransferDone;
}

s32 PollAudioSlotLoad(void) {
    s32 completed;
    register s32 *flagsPtr asm("$4");
    register s32 slot asm("$5");
    s32 one;
    s32 value;
    s32 bit;

    completed = SsVabTransCompleted(0);
    g_VabTransferDone = (s16)completed;

    if ((s16)completed != 0) {
        flagsPtr = &g_AudioLoadedSlotMask;
        one = 1;
        slot = g_AudioLoadSlot;
        value = *flagsPtr;
        bit = (s16)(one << slot);
        *flagsPtr = bit | value;

        if (slot == 0) {
            g_SoundCueBank = one;
        } else if (slot == one) {
            g_SoundCueBank = slot;
        } else {
            value = 2;
            if ((slot == value) || (slot == 3)) {
                g_SoundCueBank = value;
            }
        }
    }

    return (s16)g_VabTransferDone;
}

s32 CloseVabOnlyAudioSlot(s32 slot) {
    s32 *flagsPtr = &g_AudioLoadedSlotMask;
    s32 bit = 1;
    s32 flags = *flagsPtr;
    s32 zeroArg = 0;
    s32 ret;
    s16 *ids;

    bit <<= slot;

    if (!(bit & flags)) {
    ret = 0;
    } else {
    *flagsPtr = bit ^ flags;
    SsUtSetReverbDepth(zeroArg, 0);
    _SsVmInit(0);
    ids = g_SoundScale.vabIds;
    SsVabClose(ids[slot]);
    ret = 1;
    }
    return ret;
}

s32 CloseLoadedAudioSlots(void) {
    SpuVmDamperStep();
    if (CloseAudioSlot(1) == 0) {
        return 0;
    }
    if (CloseVabOnlyAudioSlot(2) == 0) {
        return 0;
    }
    if (CloseVabOnlyAudioSlot(3) == 0) {
        return 0;
    }
    return 1;
}

s32 StartVabTransferWithTable(u8 *header, u8 *body, u16 *table) {
    /* $18 (s2) is the one thing this shape cannot reach on its own. The slot
       pointer and `table` both want a callee-saved register, both have three
       references, and gcc's priority is refs/live-length: 3/24 for the pointer
       against 3/23 for `table`, so `table` is allocated first and takes s2.
       Retail has the pointer in s2, which needs the pointer to win. Nothing in
       the C decides that here -- the two live ranges are fixed by the call
       sequence, and every shape tried (pointer vs array vs global, local copies
       of every parameter, declaration order, the check reading the pointer or
       the global or a second local) leaves 23 against 24 unchanged. */
    register s16 *vabIdPtr asm("$18") = &g_SoundScale.vabIds[3];
    s16 vabId;

    g_AudioLoadSlot = 3;
    *vabIdPtr = SsVabOpenHeadSticky(header, -1, g_VabSpuAddress[3]);
    vabId = *vabIdPtr;
    if (vabId == -1) {
        printf("%s", g_MsgVabOpenHeadError);
        BiosExit(1);
    }

    *vabIdPtr = SsVabTransBody(body, vabId);
    if (*vabIdPtr == -1) {
        printf("%s", g_MsgVabTransBodyError);
        BiosExit(1);
    }

    if (table != 0) {
        LoadAudioParameterTable(table);
    }

    g_EngineSoundState.extraVabLoaded = 1;
    g_VabTransferDone = SsVabTransCompleted(0);
    return g_VabTransferDone;
}

s32 LoadExtraVabSlotWithTable(u8 *header, u8 *body, u16 *table) {
    /* Same allocation tie as StartVabTransferWithTable: see the note there. */
    register s16 *vabIdPtr asm("$18") = &g_SoundScale.vabIds[3];
    s16 vabId;
    s32 flags;

    *vabIdPtr = SsVabOpenHeadSticky(header, -1, 0x6A000);
    vabId = *vabIdPtr;
    if (vabId == -1) {
        printf("%s", g_MsgVabOpenHeadError);
        BiosExit(1);
    }

    *vabIdPtr = SsVabTransBody(body, vabId);
    if (*vabIdPtr == -1) {
        printf("%s", g_MsgVabTransBodyError);
        BiosExit(1);
    }

    SsVabTransCompleted(1);
    if (table != 0) {
        LoadAudioParameterTable(table);
    }

    flags = g_AudioLoadedSlotMask;
    g_EngineSoundState.extraVabLoaded = 1;
    g_AudioLoadedSlotMask = flags | 0x20;
    return 0;
}

void CloseExtraVabSlot(void) {
    s32 liveSlot = 0x15;
    s32 *flagsPtr = &g_AudioLoadedSlotMask;
    s32 flags = *flagsPtr;
    s32 newFlags;

    if (flags & 0x20) {
        newFlags = flags ^ 0x20;
        *flagsPtr = newFlags;
        SsUtReverbOff();
        SsUtSetReverbDepth(0x28, 0x28);
        SsUtKeyOffV((s16)liveSlot);
        SsVabClose(g_SoundScale.vabIds[5]);
    }
}

void ShutdownSoundSystem(void) {
    s32 i;
    s32 *flag = &g_AudioLoadedSlotMask;

    if (*flag != 0) {
        *flag = 0;
        SsUtReverbOff();
        SsUtSetReverbType(0);
        SsUtSetReverbDepth(0, 0);
        i = 0;
        while (i < 24) {
            SsUtKeyOffV((s16)i);
            i++;
        }
        VSync(2);
        SsVabClose(g_SoundScale.vabIds[4]);
        SsVabClose(g_SoundScale.vabIds[5]);
        SsStopSoundTick();
        SsQuit();
    }
}
