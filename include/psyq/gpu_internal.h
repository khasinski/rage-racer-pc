#ifndef RAGE_PC_PSYQ_GPU_INTERNAL_H
#define RAGE_PC_PSYQ_GPU_INTERNAL_H

#include "common.h"
#include "psyq/gpu.h"

extern void (*GPU_printf)(char *, ...);
extern char g_FmtGpuClearOTag[];
extern char g_FmtGpuClearOTagR[];
extern char g_FmtGpuClut[];
extern char g_FmtGpuPrimName[];
extern char g_FmtGpuRect[];
extern char g_GpuNameClearImage[];
extern char g_GpuNameLoadImage[];
extern char g_GpuNameMoveImage[];
extern char g_GpuNameStoreImage[];
extern char g_MsgGpuBadRect[];
extern char g_MsgGpuDrawSync[];
extern char g_MsgGpuDrawSyncCallback[];
extern char g_MsgGpuSetDispMask[];
extern char g_MsgGpuSetGraphQueue[];
extern char g_MsgGpuTimeout[];
extern char g_MsgGpuTimeoutCallback[];

#endif
