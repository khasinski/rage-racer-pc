#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include "port/modern/modern_assets.h"
#include "port/runtime_config.h"
#include "render/car_paint.h"
#include "../../launcher/native/preview_png.h"

size_t PortAssetRoomAt(const void *at) { (void)at; return 0; }

static int TestMaterialRetry(const char *root, const char *sidecar,
                             const RenderMeshInstance *instance) {
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
                                   const RenderMeshInstance *instance) {
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
    RenderMeshInstance instance = {0};
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
    /* Complete track banks stay immutable until the asset session retires. */
    char mods[4096],meshes[4096],manifest[4096],overridePath[4096],modSetting[4200];
    snprintf(mods,sizeof(mods),"%s/track-mods",argv[1]);
    snprintf(meshes,sizeof(meshes),"%s/meshes",mods);
    snprintf(manifest,sizeof(manifest),"%s/mod.toml",mods);
    snprintf(overridePath,sizeof(overridePath),"%s/course.rmesh",meshes);
    if(!SDL_CreateDirectory(mods)||!SDL_CreateDirectory(meshes))return 53;
    static const char trackIndex[]="# rage-rmesh-index v2\n88 course mesh.rmesh mesh.rmat\n";
    static const char trackManifest[]="[mod]\nid = \"track-test\"\n[meshes]\n\"track.big1.course\" = \"meshes/course.rmesh\"\n";
    unsigned char overrideBytes[sizeof(meshBytes)];memcpy(overrideBytes,meshBytes,sizeof(meshBytes));
    Write32(overrideBytes+32,0x40400000u);
    if(!SDL_SaveFile(path,trackIndex,sizeof(trackIndex)-1)||!SDL_SaveFile(manifest,trackManifest,sizeof(trackManifest)-1)||!SDL_SaveFile(overridePath,overrideBytes,sizeof(overrideBytes)))return 54;
    snprintf(modSetting,sizeof(modSetting),"mods.directory=%s",mods);
    char *trackArgs[]={"test","--set",modSetting,"--set","modern.authored_cars=0"};
    if(!RuntimeConfigInit(5,trackArgs)||!ModernAssetsInitRoot(argv[1]))return 55;
    instance.assetKey=88;instance.assetSet=RAGE_RENDER_ASSET_COURSE;
    resident=ModernAssetsFind(&instance);
    if(!resident||!RuntimeMeshVertex(&resident->mesh,0,&vertex)||vertex.position[0]!=3.f||ModernAssetsResidentMeshLookup(NULL,&instance)!=&resident->mesh)return 56;
    if(!SDL_SaveFile(overridePath,"invalid",7)||ModernAssetsFind(&instance)!=resident||!RuntimeMeshVertex(&resident->mesh,0,&vertex)||vertex.position[0]!=3.f)return 57;
    ModernAssetsShutdown();
    if(!ModernAssetsInitRoot(argv[1]))return 58;
    resident=ModernAssetsFind(&instance);
    if(!resident||!RuntimeMeshVertex(&resident->mesh,0,&vertex)||vertex.position[0]!=1.f)return 59;
    ModernAssetsShutdown();
    unsigned char wrongBank[36]={0};
    if(!RuntimeMeshEncodeHeader(wrongBank,sizeof(wrongBank),2,0,0)||
       !SDL_SaveFile(overridePath,wrongBank,sizeof(wrongBank))||
       !ModernAssetsInitRoot(argv[1]))return 61;
    resident=ModernAssetsFind(&instance);
    if(!resident||!RuntimeMeshVertex(&resident->mesh,0,&vertex)||vertex.position[0]!=1.f)return 62;
    ModernAssetsShutdown();
    if(!SDL_RemovePath(overridePath)||!SDL_RemovePath(manifest)||!SDL_RemovePath(meshes)||!SDL_RemovePath(mods))return 60;
    {
        char texDir[4096], carPng[4096], skyPng[4096], rmatPath[4096], paintPath[4096];
        static unsigned char paintMask[256 * 256];
        static const char hdIndex[] = "# rage-rmesh-index v2\n10 model mesh.rmesh car.rmat\n";
        static const char hdRmat[] = "# rage-rmat v6\n0 texture.rgba | car.paint | lit opaque 0.22 0.18 1 1 1 1 0 0 0\n";
        static const char hdManifest[] =
            "[mod]\nid = \"hd-test\"\n[textures]\n"
            "\"car.0.material.0\" = \"textures/car0.png\"\n"
            "\"track.big1.sky\" = \"textures/sky.png\"\n";
        SDL_Surface *carSurf = SDL_CreateSurface(1024, 1024, SDL_PIXELFORMAT_RGBA32);
        SDL_Surface *skySurf = SDL_CreateSurface(1024, 512, SDL_PIXELFORMAT_RGBA32);
        RageRenderMaterial hdDef = {0};
        RageRenderMaterialStorage hdStorage = {0};
        ModernAssetImage hdImage = {0}, skyImage = {0};
        snprintf(texDir, sizeof(texDir), "%s/textures", mods);
        snprintf(carPng, sizeof(carPng), "%s/car0.png", texDir);
        snprintf(skyPng, sizeof(skyPng), "%s/sky.png", texDir);
        snprintf(rmatPath, sizeof(rmatPath), "%s/car.rmat", argv[1]);
        snprintf(paintPath, sizeof(paintPath), "%s/car.paint", argv[1]);
        if (!carSurf || !skySurf || !SDL_CreateDirectory(mods) || !SDL_CreateDirectory(texDir)) return 63;
        memset(carSurf->pixels, 10, (size_t)carSurf->pitch * (size_t)carSurf->h);
        memset(skySurf->pixels, 200, (size_t)skySurf->pitch * (size_t)skySurf->h);
        memset(paintMask, RAGE_CAR_PAINT_NONE, sizeof(paintMask));
        paintMask[0] = RAGE_CAR_PAINT_FIRST_PRIMARY;
        if (!SDL_SavePNG(carSurf, carPng) || !SDL_SavePNG(skySurf, skyPng) ||
            !SDL_SaveFile(path, hdIndex, sizeof(hdIndex) - 1) ||
            !SDL_SaveFile(rmatPath, hdRmat, sizeof(hdRmat) - 1) ||
            !SDL_SaveFile(paintPath, paintMask, sizeof(paintMask)) ||
            !SDL_SaveFile(manifest, hdManifest, sizeof(hdManifest) - 1)) return 64;
        SDL_DestroySurface(carSurf);
        SDL_DestroySurface(skySurf);
        if (!RuntimeConfigInit(5, trackArgs) || !ModernAssetsInitRoot(argv[1])) return 65;
        instance = (RenderMeshInstance){
            .assetKey = 10,
            .assetSet = RAGE_RENDER_ASSET_MODEL_BANK,
            .hasCarPaint = 1,
            .carPaintColor1 = 3,
            .carPaintColor2 = 12,
        };
        if (!ModernAssetsLoadMaterial(&instance, 0, 0, &hdDef, &hdImage, &hdStorage) ||
            hdImage.width != 1024 || hdImage.height != 1024 ||
            ((const uint8_t *)hdImage.pixels)[0] != 148 ||
            ((const uint8_t *)hdImage.pixels)[(1 * 1024 + 1) * 4] != 148 ||
            ((const uint8_t *)hdImage.pixels)[(2 * 1024 + 2) * 4] <= 10 ||
            ((const uint8_t *)hdImage.pixels)[(2 * 1024 + 2) * 4] >= 148 ||
            ((const uint8_t *)hdImage.pixels)[(8 * 1024 + 8) * 4] != 10) return 66;
        ModernAssetsFreeMaterialImage(&hdImage);
        if (!ModernAssetsLoadSkyImage(0x58, NULL, &skyImage) ||
            skyImage.width != 1024 || skyImage.height != 512 ||
            ((const uint8_t *)skyImage.pixels)[0] != 200) return 67;
        ModernAssetsFreeMaterialImage(&skyImage);
        /* Verify 4:1 upscaled sky strip (1024x256 -> expanded 1024x512) and 512x512 cached RGBA textures. */
        {
            SDL_Surface *stripSurf = SDL_CreateSurface(1024, 256, SDL_PIXELFORMAT_RGBA32);
            if (!stripSurf) return 68;
            memset(stripSurf->pixels, 180, (size_t)stripSurf->pitch * (size_t)stripSurf->h);
            if (!SDL_SavePNG(stripSurf, skyPng)) return 68;
            SDL_DestroySurface(stripSurf);
            if (!ModernAssetsLoadSkyImage(0x58, NULL, &skyImage) ||
                skyImage.width != 1024 || skyImage.height != 512 ||
                ((const uint8_t *)skyImage.pixels)[0] != 180) return 68;
            ModernAssetsFreeMaterialImage(&skyImage);
        }
        if (!SDL_RemovePath(carPng) || !SDL_RemovePath(skyPng) ||
            !SDL_RemovePath(texDir) || !SDL_RemovePath(manifest) ||
            !SDL_RemovePath(mods)) return 68;
        ModernAssetsShutdown();
        {
            char texRgbaPath[4096];
            static unsigned char basePixels512[512 * 512 * 4];
            RenderMeshInstance worldInstance = instance;
            RenderWorld world = {0};
            snprintf(texRgbaPath, sizeof(texRgbaPath), "%s/texture.rgba", argv[1]);
            memset(basePixels512, 80, sizeof(basePixels512));
            if (!SDL_SaveFile(texRgbaPath, "bad", 3)) return 68;
            if (!ModernAssetsInitRoot(argv[1])) return 68;
            worldInstance.pass = RAGE_RENDER_PASS_MAIN;
            world.instances = &worldInstance;
            world.instanceCapacity = 1;
            world.instanceCount = 1;
            /* Failed submesh preparation must not mark the submesh prepared. */
            ModernAssetsPrepareWorld(&world);
            if (!SDL_SaveFile(texRgbaPath, basePixels512, sizeof(basePixels512))) return 68;
            ModernAssetsPrepareWorld(&world);
            /* Second prepare hits the prepared-submesh fast path; prepared
             * material remains resident even if the backing file is removed. */
            if (!SDL_RemovePath(texRgbaPath)) return 68;
            ModernAssetsPrepareWorld(&world);
            if (!ModernAssetsLoadMaterial(&instance, 0, 0, &hdDef, &hdImage, &hdStorage) ||
                hdImage.width != 512 || hdImage.height != 512 ||
                ((const uint8_t *)hdImage.pixels)[0] != 148) return 68;
            ModernAssetsFreeMaterialImage(&hdImage);
            ModernAssetsShutdown();
            if (!SDL_RemovePath(rmatPath) ||
                !SDL_RemovePath(paintPath)) return 68;
        }
    }
    if (!SDL_RemovePath(path)) return 7;
    if (!SDL_RemovePath(meshPath)) return 26;
    return 0;
}
