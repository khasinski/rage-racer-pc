#include "psyq/spu.h"
#include "psyq/spu_internal.h"


void SpuSetCommonAttr(SpuCommonAttr *attr) {
    u_short mainModeLeft;
    u_short mainModeRight;
    u_short totalLeft;
    u_short totalRight;
    u_long mask;
    long setAll;
    u_short control;

    totalLeft = 0;
    totalRight = 0;
    mask = attr->mask;
    setAll = attr->mask == 0;

    if (setAll != 0 || mask & 1) {
        if (setAll != 0 || mask & 4) {
            switch (attr->mvol.volmode.left) {
            case 1:
                mainModeLeft = 0x8000;
                break;
            case 2:
                mainModeLeft = 0x9000;
                break;
            case 3:
                mainModeLeft = 0xA000;
                break;
            case 4:
                mainModeLeft = 0xB000;
                break;
            case 5:
                mainModeLeft = 0xC000;
                break;
            case 6:
                mainModeLeft = 0xD000;
                break;
            case 7:
                mainModeLeft = 0xE000;
                break;
            case 0:
                totalLeft = attr->mvol.volume.left;
                mainModeLeft = 0;
                break;
            default:
                totalLeft = attr->mvol.volume.left;
                mainModeLeft = 0;
                break;
            }
        } else {
            totalLeft = attr->mvol.volume.left;
            mainModeLeft = 0;
        }

        if (mainModeLeft != 0) {
            if (attr->mvol.volume.left >= 0x80) {
                totalLeft = 0x7F;
            } else if (attr->mvol.volume.left < 0) {
                totalLeft = 0;
            } else {
                totalLeft = attr->mvol.volume.left;
            }
        }
        totalLeft &= 0x7FFF;
        g_SpuRegBase->regs.mainVol.left = totalLeft | mainModeLeft;
    }

    if (setAll != 0 || mask & 2) {
        if (setAll != 0 || mask & 8) {
            switch (attr->mvol.volmode.right) {
            case 1:
                mainModeRight = 0x8000;
                break;
            case 2:
                mainModeRight = 0x9000;
                break;
            case 3:
                mainModeRight = 0xA000;
                break;
            case 4:
                mainModeRight = 0xB000;
                break;
            case 5:
                mainModeRight = 0xC000;
                break;
            case 6:
                mainModeRight = 0xD000;
                break;
            case 7:
                mainModeRight = 0xE000;
                break;
            case 0:
                totalRight = attr->mvol.volume.right;
                mainModeRight = 0;
                break;
            default:
                totalRight = attr->mvol.volume.right;
                mainModeRight = 0;
                break;
            }
        } else {
            totalRight = attr->mvol.volume.right;
            mainModeRight = 0;
        }

        if (mainModeRight != 0) {
            if (attr->mvol.volume.right >= 0x80) {
                totalRight = 0x7F;
            } else if (attr->mvol.volume.right < 0) {
                totalRight = 0;
            } else {
                totalRight = attr->mvol.volume.right;
            }
        }
        totalRight &= 0x7FFF;
        g_SpuRegBase->regs.mainVol.right = totalRight | mainModeRight;
    }

    if (setAll != 0 || mask & 0x40) {
        g_SpuRegBase->regs.cdVol.left = attr->cd.volume.left;
    }

    if (setAll != 0 || mask & 0x80) {
        g_SpuRegBase->regs.cdVol.right = attr->cd.volume.right;
    }

    if (setAll != 0 || mask & 0x100) {
        if (attr->cd.reverb == 0) {
            control = g_SpuRegBase->regs.spuCnt;
            control &= ~4;
            g_SpuRegBase->regs.spuCnt = control;
        } else {
            control = g_SpuRegBase->regs.spuCnt;
            control |= 4;
            g_SpuRegBase->regs.spuCnt = control;
        }
    }

    if (setAll != 0 || mask & 0x200) {
        if (attr->cd.mix == 0) {
            control = g_SpuRegBase->regs.spuCnt;
            control &= ~1;
            g_SpuRegBase->regs.spuCnt = control;
        } else {
            control = g_SpuRegBase->regs.spuCnt;
            control |= 1;
            g_SpuRegBase->regs.spuCnt = control;
        }
    }

    if (setAll != 0 || mask & 0x400) {
        g_SpuRegBase->regs.extVol.left = attr->ext.volume.left;
    }

    if (setAll != 0 || mask & 0x800) {
        g_SpuRegBase->regs.extVol.right = attr->ext.volume.right;
    }

    if (setAll != 0 || mask & 0x1000) {
        if (attr->ext.reverb == 0) {
            control = g_SpuRegBase->regs.spuCnt;
            control &= ~8;
            g_SpuRegBase->regs.spuCnt = control;
        } else {
            control = g_SpuRegBase->regs.spuCnt;
            control |= 8;
            g_SpuRegBase->regs.spuCnt = control;
        }
    }

    if (setAll != 0 || mask & 0x2000) {
        if (attr->ext.mix == 0) {
            control = g_SpuRegBase->regs.spuCnt;
            control &= ~2;
            g_SpuRegBase->regs.spuCnt = control;
        } else {
            control = g_SpuRegBase->regs.spuCnt;
            control |= 2;
            g_SpuRegBase->regs.spuCnt = control;
        }
    }
}
