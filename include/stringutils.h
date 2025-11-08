#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <stddef.h>

void TrimNewline(char* str);
int  IsCapitalStart(const char* str);
size_t CpStringLength(const char* str);
int    CpStringContains(const char* str, const char* substr);

#endif