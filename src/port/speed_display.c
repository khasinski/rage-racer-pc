#include "rage/speed_display.h"
#include "host_disc.h"
#include <stdint.h>
#include <string.h>

int SpeedDisplayValue(int internalSpeed) {
    if (internalSpeed <= 0) return 0;
    int64_t value = (int64_t)internalSpeed * 160 / 1168;
    const char *region = HostDiscRegion();
    /* SLUS_004.03, executable file offsets 0x23e54..0x23ec4: first
     * truncate km/h, then multiply by 100 and divide by 160 for mph.
     * Preserve retail rounding rather than combining the two divisions.
     * This follows disc identity, independently of a timing override. */
    if (region && strcmp(region, "NTSC-U") == 0) value = value * 100 / 160;
    return value < 999 ? (int)value : 999;
}
