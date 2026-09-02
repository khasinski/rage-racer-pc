/*
 * Storage for the renderer's working state and the two blocks the model path
 * and the car code work in.
 *
 * A file of its own because these have to sit somewhere that does not also
 * talk to Windows: those headers declare a RECT, and so does the chain this
 * one brings in, and the two will not share a translation unit.
 */

#include <stddef.h>

#include "game/render.h"
#include "game/render_internal.h"
#include "game/car.h"
#include "game/track_internal.h"
#include "game/render_state.h"

GameRenderState g_RenderState;
ObjectMatrixWork g_ObjectMatrixWork;
CarTrackWork g_CarTrackWork;
