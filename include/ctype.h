#ifndef RAGE_PC_CTYPE_H
#define RAGE_PC_CTYPE_H

#include "common.h"

#ifdef _WIN32
int isspace(int ch);
int tolower(int ch);
int toupper(int ch);
#else
long tolower(long ch);
s32 toupper(s32 ch);
#endif

#endif
