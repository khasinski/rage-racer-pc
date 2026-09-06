#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include "port/modern/modern_assets.h"

size_t PortAssetRoomAt(const void *at) { (void)at; return 0; }

int main(int argc, char **argv) {
    char path[4096];
    char environmentPath[4096];
    const char index[] = "# rage-rmesh-index v2\n10 model mesh.rmesh mesh.rmat\n";
    if (SDL_setenv_unsafe("RAGE_PORT_MODERN_ASSETS", "", 1) != 0) return 14;
    if (argc != 2 || !SDL_CreateDirectory(argv[1])) return 1;
    if (SDL_snprintf(path, sizeof(path), "%s/runtime-index.txt", argv[1]) >= (int)sizeof(path)) return 1;
    /* This fixture owns only its index file, never a user asset directory. */
    if (!SDL_SaveFile(path, "invalid\n", 8)) return 1;
    if (ModernAssetsInitRoot(argv[1]) || ModernAssetsReady()) return 2;
    if (!SDL_SaveFile(path, index, sizeof(index) - 1)) return 3;
    if (SDL_snprintf(environmentPath, sizeof(environmentPath),
                     "%s/environment-index.txt", argv[1]) >= (int)sizeof(environmentPath)) return 10;
    /* Fail later too, after owning the successfully parsed runtime index. */
    if (!SDL_SaveFile(environmentPath, "invalid\n", 8)) return 11;
    if (ModernAssetsInitRoot(argv[1]) || ModernAssetsReady()) return 12;
    if (!SDL_RemovePath(environmentPath)) return 13;
    if (!ModernAssetsInitRoot(argv[1]) || !ModernAssetsReady()) return 4;
    /* Successful initialization is idempotent and preserves its source. */
    if (!ModernAssetsInitRoot(NULL) || !ModernAssetsReady()) return 5;
    ModernAssetsShutdown();
    ModernAssetsShutdown();
    if (ModernAssetsReady()) return 6;
    /* Offline fixture has no importer: failure must not poison InitRoot. */
    if (ModernAssetsInit() || ModernAssetsReady()) return 8;
    if (!ModernAssetsInitRoot(argv[1]) || !ModernAssetsReady()) return 9;
    ModernAssetsShutdown();
    if (SDL_setenv_unsafe("RAGE_PORT_MODERN_ASSETS", argv[1], 1) != 0) return 15;
    if (!SDL_SaveFile(path, "invalid\n", 8)) return 16;
    if (ModernAssetsInit() || ModernAssetsReady()) return 17;
    if (!SDL_SaveFile(path, index, sizeof(index) - 1)) return 18;
    if (!ModernAssetsInit() || !ModernAssetsReady()) return 19;
    if (!ModernAssetsInit()) return 20;
    ModernAssetsShutdown();
    if (!SDL_RemovePath(path)) return 7;
    return 0;
}
