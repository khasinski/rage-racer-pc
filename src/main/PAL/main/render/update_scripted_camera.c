#include "common.h"
#include "game/asset.h"
#include "game/render.h"
#include "game/render_internal.h"

/* One scripted-camera keyframe: eye position, look-at target, how long the
 * player dwells on it and the Bezier control value used to ease out of it. */

/* The retail code re-derives the keyframe address at every field access rather
 * than holding a pointer, so this stays a macro: a real pointer variable lets
 * the compiler compute the base once and drops 20 instructions. */
static __inline__ CameraKey *GetCameraKey(s32 byteOffset) {
    AssetAddress address;

    address.pointer = g_CameraPath;
    address.bytes += byteOffset;
    return address.pointer;
}


/* One frame of the scripted camera: eases the six values between the
 * current and next keyframe and installs the resulting view matrix. */
void UpdateScriptedCamera(void) {
    s32 current;
    s32 tick;
    s32 currentOffset;
    register s32 nextOffset asm("$5");
    register s32 currentValue asm("$7");
    s32 blend;
    s32 scaledTick;
    CameraLookAt camera;

    tick = g_CameraPathTick;
    current = g_CameraPathKey;
    scaledTick = tick * 10000;
    blend = BezierEase(scaledTick / GetCameraKey(current * 32)->duration,
                       GetCameraKey(current << 5)->control);

    currentOffset = g_CameraPathKey;
    nextOffset = g_CameraPathNextKey;
    currentOffset <<= 5;
    nextOffset <<= 5;

    tick = GetCameraKey(nextOffset)->eyeX;
    currentValue = GetCameraKey(currentOffset)->eyeX;
    camera.words[0] = (((tick - currentValue) * blend) / 10000) + currentValue;
    tick = GetCameraKey(nextOffset)->eyeY;
    currentValue = GetCameraKey(currentOffset)->eyeY;
    camera.words[1] = (((tick - currentValue) * blend) / 10000) + currentValue;
    tick = GetCameraKey(nextOffset)->eyeZ;
    currentValue = GetCameraKey(currentOffset)->eyeZ;
    camera.words[2] = (((tick - currentValue) * blend) / 10000) + currentValue;
    tick = GetCameraKey(nextOffset)->atX;
    currentValue = GetCameraKey(currentOffset)->atX;
    camera.words[3] = (((tick - currentValue) * blend) / 10000) + currentValue;
    tick = GetCameraKey(nextOffset)->atY;
    currentValue = GetCameraKey(currentOffset)->atY;
    camera.words[4] = (((tick - currentValue) * blend) / 10000) + currentValue;
    tick = GetCameraKey(nextOffset)->atZ;
    nextOffset = GetCameraKey(currentOffset)->atZ;
    camera.words[5] = (((tick - nextOffset) * blend) / 10000) + nextOffset;
    SetLookAtMatrix(&camera);

    {
        s32 duration = GetCameraKey(g_CameraPathKey << 5)->duration;
        s32 nextTick = g_CameraPathTick + 1;

        nextTick = (nextTick < duration) ? nextTick : 0;
        g_CameraPathTick = nextTick;
        if (nextTick == 0) {
            s32 next = g_CameraPathNextKey;
            s32 following = next + 1;

            g_CameraPathKey = next;
            g_CameraPathNextKey =
                (GetCameraKey(following << 5)->duration == -1) ? 0 : following;
        }
    }
}
