#include "game/cd.h"
#include "game/fmv_internal.h"
#include "psyq/cd.h"

void StopFmvDiscPlayback(void) {
    CdSync(0, 0);
    CdControl(CD_DRIVE_PAUSE, 0, 0);
}
