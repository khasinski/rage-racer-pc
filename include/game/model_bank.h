#ifndef GAME_MODEL_BANK_H
#define GAME_MODEL_BANK_H
#include <stddef.h>
#include "common.h"

typedef struct ModelBankHeader {
    u32 modelCount;
    s32 tableOffset;
    s32 normalsOffset;
    s32 modelOffsets[1];
} ModelBankHeader;

#define GAME_MODEL_BANK_LIMIT 16
#define GAME_MODEL_PER_BANK_LIMIT 256
typedef struct NativeModelBank {
    s32 modelCount;
    const void *table;
    const void *normals;
    const void *models[GAME_MODEL_PER_BANK_LIMIT];
} NativeModelBank;

/* Borrows immutable source bytes; failure preserves the destination. */
s32 ReadModelBank(const ModelBankHeader *base, size_t size, NativeModelBank *bank);
s32 IsValidModelBankAsset(const ModelBankHeader *base, size_t size);
typedef struct CourseModelAssetEntry {
    s32 geometryOffset;
    s32 vertexCount;
    s32 modelOffset;
} CourseModelAssetEntry;

typedef struct CourseModelAssetHeader {
    s32 modelCount;
    CourseModelAssetEntry models[1];
} CourseModelAssetHeader;

#define GAME_COURSE_MODEL_LIMIT 256
typedef struct NativeCourseModel {
    const void *geometry;
    s32 vertexCount;
    const void *model;
} NativeCourseModel;

/* Caller-owned course bank, borrowing validated source bytes. */
typedef struct CourseBank {
    s32 modelCount;
    NativeCourseModel models[GAME_COURSE_MODEL_LIMIT];
} CourseBank;
s32 IsValidCourseModelAsset(const CourseModelAssetHeader *base, size_t size);
s32 ReadCourseBank(const CourseModelAssetHeader *base, size_t size, CourseBank *bank);
#endif

