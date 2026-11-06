#include "game/car.h"
#include "game/render.h"
#include "host_disc.h"
#include "runtime_config.h"
#include "render/speed_units_poc.h"
#include <stdio.h>

void PortSpeedUnitsPrepare(void) {
    RECT faceRect = {640, 112, 24, 94};
    RECT palettesRect = {640, 206, 16, 2};
    /* Snapshot and PsyQ transfers are 32-bit aligned. */
    u_long words[RAGE_HUD_UNIT_BYTES / 4];
    uint8_t *snapshot = (uint8_t *)words;
    RageSpeedUnitsConfigure(g_PlayerCarIndex, HostDiscRegion(), RuntimeConfigGet("poc.speed_units"));
    if (!RageSpeedUnitsOverrideRequested()) return;
    DrawSync(0);
    StoreImage(&faceRect, words);
    StoreImage(&palettesRect, (u_long *)(snapshot + RAGE_HUD_FACE_BYTES));
    DrawSync(0);
    int result = RageSpeedUnitsPatchSnapshot(snapshot);
    if (!result) {
        fprintf(stderr, "rage-port: unit override rejected for car %d: unrecognized retail HUD; keeping native units\n", g_PlayerCarIndex);
        return;
    }
    if (result == 2) {
        LoadImage(&faceRect, words);
        LoadImage(&palettesRect, (u_long *)(snapshot + RAGE_HUD_FACE_BYTES));
        DrawSync(0);
    }
}
