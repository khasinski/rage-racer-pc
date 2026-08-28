#ifndef RAGE_PORT_COURSE_COORDINATE_H
#define RAGE_PORT_COURSE_COORDINATE_H

#include <stdint.h>

/* Course-object coordinates are stored modulo 65536. Recover the nearest
 * continuous coordinate around the current camera/reference position. */
static inline int32_t CourseCoordinateNearReference(int32_t encoded,
                                                        int32_t reference) {
    int32_t relative = (int16_t)((uint16_t)encoded - (uint16_t)reference);
    return reference + relative;
}

#endif
