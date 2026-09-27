#ifndef GAME_BOOT_INTERNAL_H
#define GAME_BOOT_INTERNAL_H

void DispatchCurrentScene(void);
/* Present host menu text without dispatching scenes or advancing race/save state.
 * Requires normal boot's frame buffers and menu font assets. Samples input. */
void DrawHostMenuFrame(const char *title, const char *choice,
                       const char *controls, const char *status);

#endif
