#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "common.h"

extern s32 g_LoadBuffer[];
extern u8 *g_AssetBase;
size_t PortAssetRoomAt(const void *at);

enum {
    LOAD_BUFFER_BYTES = 1037896,
    ASSET_MEMORY_BYTES = 64 * 1024 * 1024,
};

int main(void) {
    const u8 *loadBegin = (const u8 *)g_LoadBuffer;
    const u8 *assetBegin = g_AssetBase;

    assert(PortAssetRoomAt(loadBegin) == LOAD_BUFFER_BYTES);
    assert(PortAssetRoomAt(loadBegin + 1) == LOAD_BUFFER_BYTES - 1);
    assert(PortAssetRoomAt(loadBegin + LOAD_BUFFER_BYTES - 1) == 1);
    /* COFF may place the two arenas directly adjacent. The end of one is
     * then a valid start of the other, not an out-of-bounds address. */
    assert(PortAssetRoomAt(loadBegin + LOAD_BUFFER_BYTES) ==
           (loadBegin + LOAD_BUFFER_BYTES == assetBegin ? ASSET_MEMORY_BYTES : 0));
    assert(PortAssetRoomAt(assetBegin) == ASSET_MEMORY_BYTES);
    assert(PortAssetRoomAt(assetBegin + 1) == ASSET_MEMORY_BYTES - 1);
    assert(PortAssetRoomAt(assetBegin + ASSET_MEMORY_BYTES - 1) == 1);
    assert(PortAssetRoomAt(assetBegin + ASSET_MEMORY_BYTES) ==
           (assetBegin + ASSET_MEMORY_BYTES == loadBegin ? LOAD_BUFFER_BYTES : 0));
    assert(PortAssetRoomAt(NULL) == 0);
    uintptr_t first = (uintptr_t)loadBegin < (uintptr_t)assetBegin
                          ? (uintptr_t)loadBegin : (uintptr_t)assetBegin;
    uintptr_t last = (uintptr_t)loadBegin + LOAD_BUFFER_BYTES;
    if (last < (uintptr_t)assetBegin + ASSET_MEMORY_BYTES)
        last = (uintptr_t)assetBegin + ASSET_MEMORY_BYTES;
    assert(PortAssetRoomAt((const void *)(first - 1)) == 0);
    assert(PortAssetRoomAt((const void *)last) == 0);
    return 0;
}
