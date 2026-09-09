#include "disc_stage_validation.h"

#include <assert.h>
#include <string.h>

int main(void) {
    uint8_t digest[RAGE_DISC_STAGE_SHA1_BYTES];
    char hex[RAGE_DISC_STAGE_SHA1_BYTES * 2 + 1];
    unsigned char exe[0x800] = {0};

    RageDiscStageSha1("abc", 3, digest);
    RageDiscStageSha1Hex(digest, hex);
    assert(strcmp(hex, "a9993e364706816aba3e25717850c26c9cd0d89d") == 0);
    assert(!RageDiscStageValidatePsxExe(exe, sizeof(exe), hex));
    memcpy(exe, "PS-X EXE", 8);
    RageDiscStageSha1(exe, sizeof(exe), digest);
    RageDiscStageSha1Hex(digest, hex);
    assert(RageDiscStageValidatePsxExe(exe, sizeof(exe), hex));
    hex[0] = hex[0] == '0' ? '1' : '0';
    assert(!RageDiscStageValidatePsxExe(exe, sizeof(exe), hex));
    return 0;
}
