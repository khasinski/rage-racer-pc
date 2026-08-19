#ifndef RAGE_MIRROR_PASS_H
#define RAGE_MIRROR_PASS_H

/*
 * Two things reflect the view along X, and telling them apart is what the
 * renderer kept getting wrong.
 *
 * SCRATCH_MIRROR is not "the rear-view pass". It is the combined flag saying
 * the geometry being drawn is reflected, true either because the course is
 * mirrored or because BeginMirrorPass toggled it for the little mirror, and
 * false again when both hold at once, since reflecting twice is not reflecting
 * at all. The pass itself is the difference against g_MirrorMode.
 *
 * The reflection is applied in different places for the two cases. A mirrored
 * course is reflected where the camera matrix is built, before anything is
 * drawn, because the race frame draws cars before terrain. The rear view swaps
 * in a matrix of its own, so the terrain dispatcher reflects that one instead
 * and only when the course did not already supply a reflection.
 */

/* True while the little rear-view mirror is being drawn. */
int RageMirrorRearViewPass(int scratchMirror, int courseMirror);

/* True when the terrain dispatcher has to reflect the matrix it was handed. */
int RageMirrorDispatcherReflects(int scratchMirror, int courseMirror);

#endif
