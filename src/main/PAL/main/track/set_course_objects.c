#include "game/asset.h"
#include "game/track_internal.h"

void SetCourseObjects(CourseObjectTable *table) {
    g_CourseObjects = table->objects;
    g_CourseObjectCount = (s32)table->count;
}
