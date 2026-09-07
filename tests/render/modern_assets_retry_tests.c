#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include "port/modern/modern_assets.h"
#include "port/runtime_config.h"

size_t PortAssetRoomAt(const void *at) { (void)at; return 0; }

static void Write32(unsigned char *p, unsigned value) {
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
    p[2] = (unsigned char)(value >> 16);
    p[3] = (unsigned char)(value >> 24);
}

int main(int argc, char **argv) {
    char path[4096];
    char environmentPath[4096];
    char meshPath[4096];
    unsigned char meshBytes[96] = {0};
    RageRenderMeshInstance instance = {0};
    const char index[] = "# rage-rmesh-index v2\n123 model mesh.rmesh mesh.rmat\n";
    char setting[4096] = "modern.assets=disc";
    char *configArgs[] = {"asset-retry", "--set", setting};
    if (!RuntimeConfigInit(3, configArgs)) return 14;
    uint64_t initialGeneration = ModernAssetsGeneration();
    if (argc != 2 || !SDL_CreateDirectory(argv[1])) return 1;
    if (SDL_snprintf(path, sizeof(path), "%s/runtime-index.txt", argv[1]) >= (int)sizeof(path)) return 1;
    /* This fixture owns only its index file, never a user asset directory. */
    if (!SDL_SaveFile(path, "invalid\n", 8)) return 1;
    if (ModernAssetsInitRoot(argv[1]) || ModernAssetsReady()) return 2;
    if (ModernAssetsGeneration() != initialGeneration) return 31;
    if (!SDL_SaveFile(path, index, sizeof(index) - 1)) return 3;
    if (SDL_snprintf(environmentPath, sizeof(environmentPath),
                     "%s/environment-index.txt", argv[1]) >= (int)sizeof(environmentPath)) return 10;
    /* Fail later too, after owning the successfully parsed runtime index. */
    if (!SDL_SaveFile(environmentPath, "invalid\n", 8)) return 11;
    if (ModernAssetsInitRoot(argv[1]) || ModernAssetsReady()) return 12;
    if (!SDL_RemovePath(environmentPath)) return 13;
    if (!ModernAssetsInitRoot(argv[1]) || !ModernAssetsReady()) return 4;
    uint64_t liveGeneration = ModernAssetsGeneration();
    if (liveGeneration == initialGeneration) return 32;
    if (SDL_snprintf(meshPath, sizeof(meshPath), "%s/mesh.rmesh", argv[1]) >= (int)sizeof(meshPath)) return 21;
    memcpy(meshBytes, "RRMESH1", 7);
    Write32(meshBytes + 8, 1); Write32(meshBytes + 12, 1);
    Write32(meshBytes + 16, 1); Write32(meshBytes + 20, 6);
    Write32(meshBytes + 28, 6);
    if (!SDL_SaveFile(meshPath, meshBytes, sizeof(meshBytes))) return 22;
    instance.assetKey = 123;
    instance.assetSet = RAGE_RENDER_ASSET_MODEL_BANK;
    const RageRuntimeCachedMesh *resident = ModernAssetsFind(&instance);
    if (!resident || ModernAssetsCachedMeshCount() != 1) return 23;
    const void *ownedBytes = resident->mesh.bytes;
    if (!ownedBytes || memcmp(ownedBytes, meshBytes, sizeof(meshBytes))) return 24;
    /* Successful initialization is idempotent and preserves its source. */
    if (!ModernAssetsInitRoot(NULL) || !ModernAssetsReady()) return 5;
    if (ModernAssetsGeneration() != liveGeneration) return 33;
    /* An explicit different source is not an idempotent ensure-ready call.
     * Reject it without invalidating meshes borrowed by the active frame. */
    if (ModernAssetsInitRoot(meshPath) || ModernAssetsInitRoot("")) return 38;
    if (!ModernAssetsReady() || ModernAssetsGeneration() != liveGeneration ||
        ModernAssetsFind(&instance) != resident || resident->mesh.bytes != ownedBytes)
        return 39;
    if (ModernAssetsFind(&instance) != resident || resident->mesh.bytes != ownedBytes ||
        memcmp(ownedBytes, meshBytes, sizeof(meshBytes))) return 25;
    unsigned char nextMeshBytes[sizeof(meshBytes)];
    memcpy(nextMeshBytes, meshBytes, sizeof(meshBytes));
    Write32(nextMeshBytes + 32, 0x3f800000u); /* First vertex x = 1.0f. */
    if (!SDL_SaveFile(meshPath, nextMeshBytes, sizeof(nextMeshBytes))) return 27;
    /* A source edit must not mutate an already borrowed session mesh. */
    if (!ModernAssetsInitRoot(argv[1]) || ModernAssetsFind(&instance) != resident ||
        resident->mesh.bytes != ownedBytes ||
        memcmp(ownedBytes, meshBytes, sizeof(meshBytes))) return 28;
    ModernAssetsShutdown();
    uint64_t retiredGeneration = ModernAssetsGeneration();
    if (retiredGeneration == liveGeneration) return 34;
    ModernAssetsShutdown();
    if (ModernAssetsGeneration() != retiredGeneration) return 35;
    if (ModernAssetsReady() || ModernAssetsCachedMeshCount() != 0) return 6;
    /* Offline fixture has no importer: failure must not poison InitRoot. */
    if (ModernAssetsInit() || ModernAssetsReady()) return 8;
    if (ModernAssetsGeneration() != retiredGeneration) return 36;
    if (!ModernAssetsInitRoot(argv[1]) || !ModernAssetsReady()) return 9;
    if (ModernAssetsGeneration() == retiredGeneration) return 37;
    /* After full teardown, the same identity/path must resolve new bytes. */
    resident = ModernAssetsFind(&instance);
    if (!resident || memcmp(resident->mesh.bytes, nextMeshBytes, sizeof(nextMeshBytes)) ||
        ModernAssetsCachedMeshCount() != 1) return 29;
    RageRuntimeVertex vertex;
    if (!RuntimeMeshVertex(&resident->mesh, 0, &vertex) || vertex.position[0] != 1.0f)
        return 30;
    ModernAssetsShutdown();
    if (snprintf(setting, sizeof(setting), "modern.assets=%s", argv[1]) >= (int)sizeof(setting) ||
        !RuntimeConfigInit(3, configArgs)) return 15;
    if (!SDL_SaveFile(path, "invalid\n", 8)) return 16;
    if (ModernAssetsInit() || ModernAssetsReady()) return 17;
    if (!SDL_SaveFile(path, index, sizeof(index) - 1)) return 18;
    if (!ModernAssetsInit() || !ModernAssetsReady()) return 19;
    if (!ModernAssetsInit()) return 20;
    ModernAssetsShutdown();
    if (!SDL_RemovePath(path)) return 7;
    if (!SDL_RemovePath(meshPath)) return 26;
    return 0;
}
