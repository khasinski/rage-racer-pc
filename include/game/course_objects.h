#ifndef GAME_COURSE_OBJECTS_H
#define GAME_COURSE_OBJECTS_H
#include "common.h"
#include <stddef.h>

typedef struct CourseObject {
    s16 modelId;
    s16 rotationY;
    s32 x;
    s32 y;
    s32 z;
    s32 flags;
} CourseObject;

typedef enum CourseObjectFlags {
    COURSE_OBJECT_ALTERNATE_NORMAL = 1 << 0,
    COURSE_OBJECT_ALTERNATE_ENVIRONMENT_4 = 1 << 1,
    COURSE_OBJECT_ENVIRONMENT_4 = 1 << 2,
    COURSE_OBJECT_BLINK_ENVIRONMENT_4 = 1 << 3,
} CourseObjectFlags;

typedef struct CourseObjectTable {
    u32 count;
    CourseObject objects[1];
} CourseObjectTable;

typedef struct CourseObjects {
    const CourseObject *items;
    u32 count;
} CourseObjects;
/* Borrows validated aligned storage; failure preserves output. */
int ReadCourseObjects(const void *data, size_t size, s32 models, CourseObjects *objects);
#endif
