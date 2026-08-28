#include <sys/types.h>

#include "common.h"
#include "psyq/cd.h"
#include "psyq/press_internal.h"

/* StSetRing: installs the stream ring buffer (`base`, `size`) then clears it. */
void StSetRing(void *base, long size) { g_StRingBase = base; g_StRingSize = size; StClearRing(); }

/* CdGetToc: reads the disc table of contents into `toc` (thin wrapper over
 * CdGetToc2 / CdGetToc2 with track count 1). */
long CdGetToc(CdlLOC *toc) {
    return CdGetToc2(1, (u_char *)toc);
}
