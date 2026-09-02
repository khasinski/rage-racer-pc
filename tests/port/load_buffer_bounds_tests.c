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
    assert(PortAssetRoomAt(loadBegin + LOAD_BUFFER_BYTES) == 0);
    assert(PortAssetRoomAt(assetBegin) == ASSET_MEMORY_BYTES);
    assert(PortAssetRoomAt(assetBegin + 1) == ASSET_MEMORY_BYTES - 1);
    assert(PortAssetRoomAt(assetBegin + ASSET_MEMORY_BYTES - 1) == 1);
    assert(PortAssetRoomAt(assetBegin + ASSET_MEMORY_BYTES) == 0);
    assert(PortAssetRoomAt(NULL) == 0);
    assert(PortAssetRoomAt(
               (const void *)((uintptr_t)loadBegin - 1)) == 0);
    return 0;
}
