#ifndef RAGE_PC_PSYQ_GPU_INTERNAL_H
#define RAGE_PC_PSYQ_GPU_INTERNAL_H

#include "common.h"
#include "psyq/gpu_cache_types.h"
#include "psyq/gpu.h"

typedef struct GpuMovePacket {
    u_long tag;
    u_long code;
    u_long src;
    u_long dst;
    u_long wh;
} GpuMovePacket;

extern u_long g_ClearImagePacket[];
extern GpuDisplayEnvironmentCache g_DispEnvCache;
extern u_char g_DrawSyncCbPending[];
extern volatile long g_DrawSyncCallback;
extern void (*GPU_printf)(char *, ...);
extern GpuCallbacks *g_GpuFuncs;
extern u_char g_GraphDebug;
extern volatile u_char g_GraphQueue;
extern u_char g_GraphReverse;
extern u_char g_GraphType;
extern short g_VramHeight;
extern short g_VramWidth;
extern char g_FmtGpuClearOTag[];
extern char g_FmtGpuClearOTagR[];
extern char g_FmtGpuClut[];
extern char g_FmtGpuPrimName[];
extern char g_FmtGpuRect[];
extern char g_GpuNameClearImage[];
extern char g_GpuNameLoadImage[];
extern char g_GpuNameMoveImage[];
extern char g_GpuNameStoreImage[];
extern volatile u_long *g_GpuGp1;
extern volatile u_long *g_GpuDmaBcr;
extern volatile u_long *g_GpuDmaChcr;
extern volatile u_long *g_GpuDmaMadr;
extern volatile u_long *g_GpuDpcr;
extern volatile u_long *g_GpuGp0;
extern u_char g_GpuGp1Mirror[];
extern char g_MsgGpuBadRect[];
extern char g_MsgGpuDrawSync[];
extern char g_MsgGpuDrawSyncCallback[];
extern char g_MsgGpuSetDispMask[];
extern char g_MsgGpuSetGraphQueue[];
extern char g_MsgGpuTimeout[];
extern char g_MsgGpuTimeoutCallback[];
extern GpuMovePacket g_MoveImagePacket;
extern volatile u_long *g_OtcDmaBcr;
extern volatile u_long *g_OtcDmaChcr;
extern volatile u_long *g_OtcDmaMadr;
extern u_long g_OtagTerminator;

#endif
