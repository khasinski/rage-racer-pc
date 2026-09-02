#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "common.h"

extern s32 g_LoadBuffer[];
unsigned long PortLoadBufferRoomAt(const void *at);

enum { LOAD_BUFFER_BYTES = 1037896 };

int main(void) {
    const u8 *begin = (const u8 *)g_LoadBuffer;

    assert(PortLoadBufferRoomAt(begin) == LOAD_BUFFER_BYTES);
    assert(PortLoadBufferRoomAt(begin + 1) == LOAD_BUFFER_BYTES - 1);
    assert(PortLoadBufferRoomAt(begin + LOAD_BUFFER_BYTES - 1) == 1);
    assert(PortLoadBufferRoomAt(begin + LOAD_BUFFER_BYTES) == 0);
    assert(PortLoadBufferRoomAt(NULL) == 0);
    assert(PortLoadBufferRoomAt(
               (const void *)((uintptr_t)begin - 1)) == 0);
    return 0;
}
