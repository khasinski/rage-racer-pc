#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include "port/modern/modern_assets.h"
#include "port/runtime_config.h"

size_t PortAssetRoomAt(const void *at) { (void)at; return 0; }

static int TestMaterialRetry(const char *root, const char *sidecar,
                             const RageRenderMeshInstance *instance) {
    char path[4096], pixelsPath[4096];
    static const char material[] = "# rage-rmat v4\n0 texture.rgba\n";
    static unsigned char pixels[256 * 256 * 4];
    RageRenderMaterial definition = {0};
    RageRenderMaterialStorage storage = {0};
    ModernAssetImage image = {0};
    if (SDL_snprintf(path, sizeof(path), "%s/%s", root, sidecar) >= (int)sizeof(path) ||
        SDL_snprintf(pixelsPath, sizeof(pixelsPath), "%s/texture.rgba", root) >= (int)sizeof(pixelsPath)) return 0;
    memset(pixels, 173, sizeof(pixels));
    /* Failed catalog validation must be retryable, including a valid target
     * followed by a malformed unrelated record. */
    static const char invalid[] = "# rage-rmat v4\n0 texture.rgba\ninvalid\n";
    if (!SDL_SaveFile(path, invalid, sizeof(invalid) - 1) ||
        !SDL_SaveFile(pixelsPath, pixels, sizeof(pixels))) return 0;
    if (ModernAssetsLoadMaterial(instance, 0, 0, &definition, &image, &storage) ||
        image.pixels || storage.baseColorTexture[0]) return 0;
    if (!SDL_SaveFile(path, material, sizeof(material) - 1)) return 0;
    if (!ModernAssetsLoadMaterial(instance, 0, 0, &definition, &image, &storage)) return 0;
    if (image.size != sizeof(pixels) || memcmp(image.pixels, pixels, sizeof(pixels)) ||
        definition.baseColorTexture.text != storage.baseColorTexture ||
        strcmp(storage.baseColorTexture, "texture.rgba")) return 0;
    RageRenderMaterial savedDefinition = definition;
    RageRenderMaterialStorage savedStorage = storage;
    ModernAssetsFreeMaterialImage(&image);
    if (!SDL_SaveFile(path, "invalid", 7) ||
        !SDL_SaveFile(pixelsPath, pixels, 1)) return 0;
    /* A prepared material is immutable for this asset session. Broken files
     * after the first successful read must not cause a first-draw hitch or
     * replace the material under a live renderer. */
    if (!ModernAssetsLoadMaterial(instance, 0, 0, &definition, &image, &storage) ||
        memcmp(&definition, &savedDefinition, sizeof(definition)) ||
        memcmp(&storage, &savedStorage, sizeof(storage)) ||
        memcmp(image.pixels, pixels, sizeof(pixels))) return 0;
    ModernAssetsFreeMaterialImage(&image);
    /* Repair only the pixels: the catalog still supplies the validated
     * definition even though its source file has changed. */
    if (!SDL_SaveFile(pixelsPath, pixels, sizeof(pixels)) ||
        !ModernAssetsLoadMaterial(instance, 0, 0, &definition, &image, &storage)) return 0;
    ModernAssetsFreeMaterialImage(&image);
    return SDL_RemovePath(path) && SDL_RemovePath(pixelsPath);
}

static void Write32(unsigned char *p, unsigned value) {
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
    p[2] = (unsigned char)(value >> 16);
    p[3] = (unsigned char)(value >> 24);
}

static int TestMaterialRetirement(const char *root,
                                   const RageRenderMeshInstance *instance) {
    char sidecar[4096], texture[4096];
    static unsigned char pixels[256 * 256 * 4];
    static const char updated[] = "# rage-rmat v6\n"
        "0 next.rgba | - | unlit opaque 0.25 0 1 1 1 1 0 0 0\n";
    RageRenderMaterial definition = {0};
    RageRenderMaterialStorage storage = {0};
    ModernAssetImage image = {0};
    if (SDL_snprintf(sidecar, sizeof(sidecar), "%s/-material", root) >= (int)sizeof(sidecar) ||
        SDL_snprintf(texture, sizeof(texture), "%s/next.rgba", root) >= (int)sizeof(texture)) return 0;
    memset(pixels, 219, sizeof(pixels));
    if (!SDL_SaveFile(sidecar, updated, sizeof(updated) - 1) ||
        !SDL_SaveFile(texture, pixels, sizeof(pixels))) return 0;
    /* The old material was prepared by the active session. The new sidecar
     * must not leak in until the session retires, even though its source
     * texture was removed by the preceding retry test. */
    if (!ModernAssetsLoadMaterial(instance, 0, 0, &definition, &image, &storage) ||
        strcmp(storage.baseColorTexture, "texture.rgba")) return 0;
    ModernAssetsFreeMaterialImage(&image);
    uint64_t generation = ModernAssetsGeneration();
    ModernAssetsShutdown();
    if (!ModernAssetsInitRoot(root) || ModernAssetsGeneration() == generation ||
        !ModernAssetsLoadMaterial(instance, 0, 0, &definition, &image, &storage) ||
        definition.roughness != 0.25f || definition.shading != RAGE_RENDER_MATERIAL_SHADING_UNLIT ||
        strcmp(storage.baseColorTexture, "next.rgba") ||
        image.size != sizeof(pixels) || memcmp(image.pixels, pixels, sizeof(pixels))) return 0;
    /* Public results retain their own paths and pixels after session teardown. */
    ModernAssetsShutdown();
    if (strcmp(definition.baseColorTexture.text, "next.rgba") ||
        memcmp(image.pixels, pixels, sizeof(pixels))) return 0;
    ModernAssetsFreeMaterialImage(&image);
    return SDL_RemovePath(sidecar) && SDL_RemovePath(texture);
}

int main(int argc, char **argv) {
    char path[4096];
    char environmentPath[4096];
    char meshPath[4096];
    char secondRoot[4096], secondIndex[4096], secondMesh[4096];
    unsigned char meshBytes[96] = {0};
    RageRenderMeshInstance instance = {0};
    const char index[] = "# rage-rmesh-index v2\n123 model mesh.rmesh m\n";
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
    /* A valid header/first record must not hide malformed later records or
     * duplicate identities. Reject the entire source before publishing it. */
    static const char *invalidIndexes[] = {
        "# rage-rmesh-index v2\n123 model mesh.rmesh mesh.rmat\n124 model ../bad.rmesh mesh.rmat\n",
        "# rage-rmesh-index v2\n123 model mesh.rmesh mesh.rmat\n123 model other.rmesh mesh.rmat\n",
        "# rage-rmesh-index v2\n123 model mesh.rmesh mesh.rmat\ninvalid\n"
    };
    for (size_t i = 0; i < sizeof(invalidIndexes) / sizeof(invalidIndexes[0]); ++i) {
        if (!SDL_SaveFile(path, invalidIndexes[i], strlen(invalidIndexes[i]))) return 48;
        if (ModernAssetsInitRoot(argv[1]) || ModernAssetsReady() ||
            ModernAssetsGeneration() != initialGeneration ||
            ModernAssetsCachedMeshCount() != 0) return 49;
    }
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
    if (!TestMaterialRetry(argv[1], "m", &instance)) return 50;
    if (SDL_snprintf(secondRoot, sizeof(secondRoot), "%s/second", argv[1]) >= (int)sizeof(secondRoot) ||
        SDL_snprintf(secondIndex, sizeof(secondIndex), "%s/runtime-index.txt", secondRoot) >= (int)sizeof(secondIndex) ||
        SDL_snprintf(secondMesh, sizeof(secondMesh), "%s/mesh.rmesh", secondRoot) >= (int)sizeof(secondMesh) ||
        !SDL_CreateDirectory(secondRoot)) return 40;
    unsigned char secondBytes[sizeof(meshBytes)];
    memcpy(secondBytes, meshBytes, sizeof(secondBytes));
    Write32(secondBytes + 32, 0x40000000u); /* Same asset identity, x = 2. */
    static const char secondIndexText[] = "# rage-rmesh-index v2\n123 model mesh.rmesh -material\n";
    if (!SDL_SaveFile(secondIndex, secondIndexText, sizeof(secondIndexText) - 1) ||
        !SDL_SaveFile(secondMesh, secondBytes, sizeof(secondBytes))) return 41;
    /* Successful initialization is idempotent and preserves its source. */
    if (!ModernAssetsInitRoot(NULL) || !ModernAssetsReady()) return 5;
    if (ModernAssetsGeneration() != liveGeneration) return 33;
    /* An explicit different source is not an idempotent ensure-ready call.
     * Reject it without invalidating meshes borrowed by the active frame. */
    if (ModernAssetsInitRoot(meshPath) || ModernAssetsInitRoot("") ||
        ModernAssetsInitRoot(secondRoot)) return 38;
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
    uint64_t configuredGeneration = ModernAssetsGeneration();
    if (snprintf(setting, sizeof(setting), "modern.assets=%s", secondRoot) >= (int)sizeof(setting) ||
        !RuntimeConfigInit(3, configArgs)) return 45;
    if (ModernAssetsInit() || !ModernAssetsReady() ||
        ModernAssetsGeneration() != configuredGeneration) return 46;
    strcpy(setting, "modern.assets=disc");
    if (!RuntimeConfigInit(3, configArgs) || ModernAssetsInit() ||
        !ModernAssetsReady() || ModernAssetsGeneration() != configuredGeneration)
        return 47;
    ModernAssetsShutdown();
    /* The valid second source becomes available only after retiring the
     * first session. No cache entry may survive merely because IDs match. */
    uint64_t beforeSwitch = ModernAssetsGeneration();
    if (!ModernAssetsInitRoot(secondRoot) || !ModernAssetsReady() ||
        ModernAssetsGeneration() == beforeSwitch) return 42;
    resident = ModernAssetsFind(&instance);
    if (!resident || !RuntimeMeshVertex(&resident->mesh, 0, &vertex) ||
        vertex.position[0] != 2.0f || ModernAssetsCachedMeshCount() != 1)
        return 43;
    if (!TestMaterialRetry(secondRoot, "-material", &instance)) return 51;
    if (!TestMaterialRetirement(secondRoot, &instance)) return 52;
    ModernAssetsShutdown();
    if (!SDL_RemovePath(secondIndex) || !SDL_RemovePath(secondMesh) ||
        !SDL_RemovePath(secondRoot)) return 44;
    if (!SDL_RemovePath(path)) return 7;
    if (!SDL_RemovePath(meshPath)) return 26;
    return 0;
}
