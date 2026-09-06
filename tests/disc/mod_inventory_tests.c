#include <SDL3/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "launcher/native/mod_inventory.h"

int main(void) {
    char root[128], path[256], hidden[256];
    snprintf(root, sizeof(root), "inventory_\xc4\x85_%llu", (unsigned long long)SDL_GetTicksNS());
    assert(SDL_CreateDirectory(root));
    assert(InventoryCommand(root) == 1);
    snprintf(path, sizeof(path), "%s/mod.toml", root);
    assert(SDL_SaveFile(path, "[mod]", 5));
    assert(InventoryCommand(root) == 0);
    snprintf(hidden, sizeof(hidden), "%s/.ignored", root);
    assert(SDL_SaveFile(hidden, "ignored", 7));
    assert(InventoryCommand(root) == 0);
    snprintf(path, sizeof(path), "%s/textures", root);
    assert(SDL_CreateDirectory(path));
    assert(InventoryCommand(root) == 0);
    snprintf(path, sizeof(path), "%s/textures/bad name", root);
    assert(SDL_CreateDirectory(path));
    assert(InventoryCommand(root) == 1);
    assert(SDL_RemovePath(path));
    snprintf(path, sizeof(path), "%s/textures/a.png", root);
    assert(SDL_SaveFile(path, "image", 5));
    assert(InventoryCommand(root) == 0);
    assert(SDL_RemovePath(path));
    snprintf(path, sizeof(path), "%s/run.js", root);
    assert(SDL_SaveFile(path, "no", 2));
    assert(InventoryCommand(root) == 1);
    assert(SDL_RemovePath(path));
    snprintf(path, sizeof(path), "%s/textures", root); assert(SDL_RemovePath(path));
    snprintf(path, sizeof(path), "%s/mod.toml", root); assert(SDL_RemovePath(path));
    assert(SDL_RemovePath(hidden));
    assert(SDL_RemovePath(root));
    return 0;
}
