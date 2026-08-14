#ifndef RAGE_PC_STRING_H
#define RAGE_PC_STRING_H

#include "common.h"

#ifdef _WIN32
void *memchr(const void *buf, int ch, unsigned long long len);
void *memcpy(void *dest, const void *src, unsigned long long count);
void *memmove(void *dest, const void *src, unsigned long long count);
int strcmp(const char *lhs, const char *rhs);
int strncmp(const char *lhs, const char *rhs, unsigned long long len);
unsigned long long strlen(const char *str);
char *strchr(const char *str, int ch);
#else
u8 *memchr(u8 *buf, s32 ch, s32 len);
void memcpy(u8 *dest, u8 *src, long count);
void *memmove(u8 *dest, u8 *src, s32 count);
long strcmp(u8 *lhs, u8 *rhs);
long strncmp(u8 *lhs, u8 *rhs, long len);
s32 strlen(u8 *str);
#endif

#endif
