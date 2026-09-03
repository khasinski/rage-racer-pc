#ifndef GAME_VECTOR_H
#define GAME_VECTOR_H

#include "common.h"

/* libgte's fixed-point square root, declared here so this header stays
 * usable by hosts that do not link the GTE. */
long SquareRoot12(long a);

/*
 * Planar distance between two points, in the fixed point SquareRoot12 returns.
 *
 * Squaring a coordinate difference overflows a signed 32-bit int once the two
 * points are roughly 46000 apart, and the sum then reaches SquareRoot12 as a
 * negative number. The finish camera did exactly that fifty times over the two
 * seconds before it crashed, and the only sign of it was a warning nobody
 * reads. Widening the intermediate and scaling by whole powers of four keeps
 * the answer exact across the range that used to wrap: dividing a square by
 * four halves its root, so shifting the result back is lossless.
 *
 * Callers that already divide their squares by hand, as the scenery walkers
 * do, were working around this cliff; they can lose the divisions.
 */
static inline s32 DistanceXZ(s32 dx, s32 dz) {
    long long squared = (long long)dx * dx + (long long)dz * dz;
    s32 shift = 0;
    while (squared > 0x7FFFFFFF) {
        squared >>= 2;
        shift++;
    }
    return (s32)(SquareRoot12((long)squared) << shift);
}

/* Word-sized position/velocity vector. */
typedef struct Vec4 {
    s32 x;
    s32 y;
    s32 z;
    s32 w;
} Vec4;

/* Half-word position/rotation vector; the GTE's SVECTOR shape. */
typedef struct SVec {
    s16 vx;
    s16 vy;
    s16 vz;
    s16 pad;
} SVec;

/* Half-word screen/point pair; the GTE's DVECTOR shape. */
typedef struct DVec {
    s16 vx;
    s16 vy;
} DVec;

typedef union DVecValue {
    DVec components;
    s32 packed;
} DVecValue;

/* Eight-bit colour triplet. */
typedef struct Rgb {
    u8 r;
    u8 g;
    u8 b;
} Rgb;

/* Colour plus GPU command byte; the SDK's CVECTOR shape. */
typedef struct CVec {
    u8 r;
    u8 g;
    u8 b;
    u8 cd;
} CVec;

/* Word position vector without a fourth word. */
typedef struct LVec {
    s32 x;
    s32 y;
    s32 z;
} LVec;

/*
 * A Vec4 is an LVec with a fourth word after it, so anything that wants only a
 * position can be handed one. The conversion is sound; making it in silence,
 * through a pointer the compiler has been told not to look at, is what was
 * not. Every caller that needs it now says so.
 */
/*
 * Three words in a row are a position. Scenery, waypoints and render objects
 * all begin with one and are handed to routines that want only that, and the
 * scratch arrays the model code passes are the same three words without a name
 * on them. Writing the conversion out says which words are meant.
 */
/*
 * The other way round: a routine that takes the words rather than the vector.
 * ApplyMatrixLV is handed both scratch arrays and named vectors by its
 * fourteen callers, so the named ones say here that they mean their words.
 */
static inline s32 *AsWords(Vec4 *vector) {
    union {
        Vec4 *four;
        s32 *words;
    } view;

    view.four = vector;
    return view.words;
}

static inline const LVec *AsPositionWords(const s32 *words) {
    union {
        const s32 *words;
        const LVec *position;
    } view;

    view.words = words;
    return view.position;
}

static inline LVec *AsPosition(Vec4 *vector) {
    union {
        Vec4 *four;
        LVec *three;
    } view;

    view.four = vector;
    return view.three;
}

/* Sixteen bytes moved as a unit; also indexed a word at a time. */
typedef struct Block16 {
    s32 w[4];
} Block16;

#endif
