#include "common.h"
#include "game/render.h"
#include "game/track_internal.h"

void SetEnvPaletteTable(void *table) {
    g_EnvPaletteTable = table;
}

void LerpEnvColor(GameEnvColor *from, GameEnvColor *to, GameEnvColor *out,
                  s32 blend) {
    out->bytes.r = LerpColorChannel(from->bytes.r, to->bytes.r, blend);
    out->bytes.g = LerpColorChannel(from->bytes.g, to->bytes.g, blend);
    out->bytes.b = LerpColorChannel(from->bytes.b, to->bytes.b, blend);
}
