#ifndef RAGE_PC_STDIO_H
#define RAGE_PC_STDIO_H

#include "common.h"

#ifdef _WIN32
typedef struct _iobuf FILE;
int printf(const char *format, ...);
int sprintf(char *dest, const char *format, ...);
FILE *fopen(const char *path, const char *mode);
char *fgets(char *buffer, int size, FILE *stream);
int fclose(FILE *stream);
int fprintf(FILE *stream, const char *format, ...);
int snprintf(char *buffer, unsigned long long size, const char *format, ...);
FILE *__acrt_iob_func(unsigned index);
#define stderr (__acrt_iob_func(2))
#else
s32 printf(const char *format, ...);
s32 sprintf(char *dest, const char *format, ...);
void putchar(s32 ch);
void puts(u8 *str);
#endif

#endif
