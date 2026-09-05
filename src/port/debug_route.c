#include "debug_route.h"

int DebugRouteAdvance(DebugRoute *r, int count, int direction, int speed,
                      int hz, DebugRouteRead read, void *context,
                      DebugRouteSample *out) {
    int distance;
    if (!r || !read || !out || count < 2 || count > 65535 || r->point < 0 ||
        r->point >= count || (direction != 1 && direction != -1) ||
        speed < 0 || speed > 100000 || hz < 1 || hz > 1000 ||
        r->offset < 0 || r->offset > 65535 || r->segments < 0 ||
        r->segments >= count || r->remainder < 0 || r->remainder >= hz) return 0;
    r->remainder += speed;
    distance = r->remainder / hz;
    r->remainder %= hz;
    r->distance += (uint64_t)distance;
    r->offset += distance;
    for (;;) {
        int next = (r->point + direction + count) % count;
        DebugRoutePoint a = read(context, r->point), b = read(context, next);
        int length = direction > 0 ? a.length : b.length;
        int angle;
        if (length <= 0 || length > 65535) return 0;
        if (r->offset >= length) {
            r->offset -= length;
            r->point = next;
            if (++r->segments == count) { r->segments = 0; r->laps++; }
            continue;
        }
        out->x = (int)((int64_t)a.x + ((int64_t)b.x - a.x) * r->offset / length);
        out->z = (int)((int64_t)a.z + ((int64_t)b.z - a.z) * r->offset / length);
        angle = ((b.angle - a.angle + 2048) & 4095) - 2048;
        angle = a.angle + angle * r->offset / length;
        out->heading = ((direction > 0 ? 0x400 : 0xc00) - angle) & 4095;
        return 1;
    }
}
