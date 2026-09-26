#include "game/model_bank.h"
#include "game/vector.h"
#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)

int main(void) {
    struct {
        u32 count;
        s32 table, normals, models[2];
        u32 payload[12];
    } source = {2, 24, 28, {32, 36}, {0}}, other = source;
    const ModelBankHeader *header = (const ModelBankHeader *)&source;
    NativeModelBank first, second;
    memset(&first, 0xA5, sizeof(first));
    CHECK(ReadModelBank(header, sizeof(source), &first));
    CHECK(first.modelCount == 2);
    CHECK(first.table == (const u8 *)&source + 24);
    CHECK(first.normals == (const u8 *)&source + 28);
    CHECK(first.models[0] == (const u8 *)&source + 32);
    CHECK(first.models[1] == (const u8 *)&source + 36);
    CHECK(first.models[2] == NULL);
    CHECK(ReadModelBank((const ModelBankHeader *)&other, sizeof(other), &second));
    CHECK(second.models[0] == (const u8 *)&other + 32);
    CHECK(second.models[0] != first.models[0]);
    NativeModelBank saved = first;
    for (size_t size = 0; size < 40; size++) {
        CHECK(!ReadModelBank(header, size, &first));
        CHECK(memcmp(&first, &saved, sizeof(first)) == 0);
    }
    source.count = UINT32_MAX;
    CHECK(!ReadModelBank(header, sizeof(source), &first));
    source.count = 2;
    source.models[1] = 37;
    CHECK(!ReadModelBank(header, sizeof(source), &first));
    source.models[1] = sizeof(source);
    CHECK(!ReadModelBank(header, sizeof(source), &first));
    source.models[1] = 36;
    source.table = -1;
    CHECK(!ReadModelBank(header, sizeof(source), &first));
    source.table = 24;
    /* Unsupported primitive with a nonempty batch. */
    source.payload[(32 - 20) / 4] = 0x000100FF;
    CHECK(!ReadModelBank(header, sizeof(source), &first));
    CHECK(memcmp(&first, &saved, sizeof(first)) == 0);
    source.payload[(32 - 20) / 4] = 0;
    CHECK(!ReadModelBank(NULL, sizeof(source), &first));
    CHECK(!ReadModelBank(header, sizeof(source), NULL));
    CHECK(memcmp(&first, &saved, sizeof(first)) == 0);
    source.count = 1;
    CHECK(ReadModelBank(header, sizeof(source), &first));
    CHECK(first.modelCount == 1 && first.models[1] == NULL);
    CHECK(second.modelCount == 2 && second.models[1] != NULL);
/* Valid stream lengths alone must not permit indexed reads past storage. */
struct {
    u32 count;
    s32 table, normals, model;
    u16 stream[20];
    u32 vectors[16];
} indexed = {.count = 1, .table = 56, .normals = 56, .model = 16};
indexed.stream[0] = 2; indexed.stream[1] = 1;
NativeModelBank resolved = {0};
CHECK(ReadModelBank((const ModelBankHeader *)&indexed, sizeof(indexed), &resolved));
NativeModelBank valid = resolved;
const u16 capacity = (sizeof(indexed) - 56) / 8;
for (unsigned corner = 0; corner < 4; ++corner) {
    indexed.stream[2 + corner] = capacity - 1;
    CHECK(ReadModelBank((const ModelBankHeader *)&indexed, sizeof(indexed), &resolved));
    valid = resolved;
    indexed.stream[2 + corner] = capacity;
    CHECK(!ReadModelBank((const ModelBankHeader *)&indexed, sizeof(indexed), &resolved));
    CHECK(memcmp(&resolved, &valid, sizeof(valid)) == 0);
    indexed.stream[2 + corner] = 0;
    indexed.stream[6 + corner] = capacity - 1;
    CHECK(ReadModelBank((const ModelBankHeader *)&indexed, sizeof(indexed), &resolved));
    valid = resolved;
    indexed.stream[6 + corner] = capacity;
    CHECK(!ReadModelBank((const ModelBankHeader *)&indexed, sizeof(indexed), &resolved));
    CHECK(memcmp(&resolved, &valid, sizeof(valid)) == 0);
    indexed.stream[6 + corner] = 0;
}
struct {
    s32 count;
    CourseModelAssetEntry entry;
    SVec vertices[4];
    u16 stream[12];
} course = {.count = 1, .entry = {16, 4, 48}};
course.stream[1] = 1;
course.stream[2] = 0; course.stream[3] = 1;
course.stream[4] = 2; course.stream[5] = 3;
CourseBank bank;
CHECK(ReadCourseBank((const void *)&course, sizeof(course), &bank));
CHECK(bank.modelCount == 1 && bank.models[0].geometry == course.vertices);
CHECK(bank.models[0].model == course.stream && bank.models[0].vertexCount == 4);
const CourseBank savedCourse = bank;
course.entry.vertexCount = INT32_MAX;
CHECK(!ReadCourseBank((const void *)&course, sizeof(course), &bank));
CHECK(memcmp(&bank, &savedCourse, sizeof(bank)) == 0);
course.entry.vertexCount = 4;
course.stream[5] = 4; /* Indexed vertex beyond the authored count. */
CHECK(!ReadCourseBank((const void *)&course, sizeof(course), &bank));
CHECK(memcmp(&bank, &savedCourse, sizeof(bank)) == 0);
course.stream[5] = 3;
CHECK(!ReadCourseBank((const void *)&course, 48, &bank));
CHECK(!ReadCourseBank(NULL, sizeof(course), &bank));
CHECK(!ReadCourseBank((const void *)&course, sizeof(course), NULL));
CHECK(memcmp(&bank, &savedCourse, sizeof(bank)) == 0);
return 0;


}
