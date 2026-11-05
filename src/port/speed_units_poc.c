#include "game/car.h"
#include "game/render.h"
#include "host_disc.h"
#include "runtime_config.h"
#include "render/speed_units_poc.h"

void PortSpeedUnitsPrepare(void) {
    RECT faceRect = {640, 112, 24, 94};
    RECT paletteRect = {640, 206, 16, 1};
    /* 32-bit alignment for PsyQ transfers. */
    u_long face[24 * 94 / 2], palette[8];
    RageSpeedUnitsConfigure(g_PlayerCarIndex, HostDiscRegion(),
                           RuntimeConfigGet("poc.speed_units"));
    if (RageSpeedUnits() != RAGE_UNITS_MPH) return;
    DrawSync(0);
    StoreImage(&faceRect, face);
    StoreImage(&paletteRect, palette);
    DrawSync(0);
    RageSpeedUnitsPatchIndexed((uint16_t *)face, (uint16_t *)palette);
    LoadImage(&faceRect, face);
    DrawSync(0);
}
