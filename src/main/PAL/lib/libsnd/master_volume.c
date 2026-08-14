#include "psyq/spu.h"


void SsSetMVol(short left, short right) {
    long left_s = left;
    long right_s = right;
    SpuCommonAttr attr;

    attr.mask = 3;
    attr.mvol.volume.left = (left_s << 7) + left_s;
    attr.mvol.volume.right = (right_s << 7) + right_s;
    SpuSetCommonAttr(&attr);
}


void SsSetSpuInputAttr(u_char source, u_char field, u_char value) {
    SpuCommonAttr attr;

    if (source == 0) {
        if (field == 0) {
            attr.mask = 0x200;
            attr.cd.mix = value;
        }
        if (field == 1) {
            attr.mask = 0x100;
            attr.cd.reverb = value;
        }
    }

    if (source == 1) {
        if (field == 0) {
            attr.mask = 0x2000;
            attr.ext.mix = value;
        }
        if (field == 1) {
            attr.mask = 0x1000;
            attr.ext.reverb = value;
        }
    }

    SpuSetCommonAttr(&attr);
}


void SsSetSerialVol(u_char source, short left, short right) {
    SpuCommonAttr attr;

    if (source == 0) {
        attr.mask = 0xC0;
        if (left >= 0x80) {
            left = 0x7F;
        }
        if (right >= 0x80) {
            right = 0x7F;
        }
        attr.cd.volume.left = left * 0x102;
        attr.cd.volume.right = right * 0x102;
    }

    if (source == 1) {
        attr.mask = 0xC00;
        if (left >= 0x80) {
            left = 0x7F;
        }
        if (right >= 0x80) {
            right = 0x7F;
        }
        attr.ext.volume.left = left * 0x102;
        attr.ext.volume.right = right * 0x102;
    }

    SpuSetCommonAttr(&attr);
}
