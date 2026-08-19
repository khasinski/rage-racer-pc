#include "include/mirror_pass.h"

int RageMirrorRearViewPass(int scratchMirror, int courseMirror) {
    return (scratchMirror != 0) != (courseMirror != 0);
}

int RageMirrorDispatcherReflects(int scratchMirror, int courseMirror) {
    /* Only the rear view, and only when the course has not already reflected
     * things: a rear view of a mirrored course is two reflections, which is
     * none. */
    return scratchMirror != 0 && courseMirror == 0;
}
