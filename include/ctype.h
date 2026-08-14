#ifndef RAGE_PC_CTYPE_H
#define RAGE_PC_CTYPE_H

#ifdef RAGE_HOST_PORT
#include_next <ctype.h>
#else

#include "common.h"

#ifdef _WIN32
int isspace(int ch);
int isalpha(int ch);
int tolower(int ch);
int toupper(int ch);
#else
long tolower(long ch);
s32 toupper(s32 ch);
#endif

#endif

#endif
