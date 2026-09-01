#ifndef TEST_SAVE_ICON_PSYQ_GPU_H
#define TEST_SAVE_ICON_PSYQ_GPU_H

#include "psyq/gpu_types.h"

void StoreImage(Rect *rect, void *data);
void DrawSync(long mode);

#endif
