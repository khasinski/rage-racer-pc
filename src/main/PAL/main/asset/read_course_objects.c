#include "game/course_objects.h"
#include <stdint.h>

int ReadCourseObjects(const void *data, size_t size, s32 models, CourseObjects *objects) {
    if (!data || !objects || models < 0 || (uintptr_t)data % _Alignof(CourseObjectTable) ||
        size < offsetof(CourseObjectTable, objects)) return 0;
    const CourseObjectTable *table = data;
    if (table->count > INT32_MAX || table->count >
        (size - offsetof(CourseObjectTable, objects)) / sizeof(CourseObject)) return 0;
    const s32 flags = COURSE_OBJECT_ALTERNATE_NORMAL | COURSE_OBJECT_ALTERNATE_ENVIRONMENT_4 |
        COURSE_OBJECT_ENVIRONMENT_4 | COURSE_OBJECT_BLINK_ENVIRONMENT_4;
    for (u32 i = 0; i < table->count; ++i) {
        const CourseObject *object = &table->objects[i];
        if ((object->modelId != -1 && (object->modelId < 0 || object->modelId >= models)) ||
            (object->flags & ~flags)) return 0;
    }
    *objects = (CourseObjects){table->objects, table->count};
    return 1;
}
