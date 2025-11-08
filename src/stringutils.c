#include "stringutils.h"
#include <string.h>
#include <ctype.h>

void TrimNewline(char* str) {
    if (!str) return;
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') str[len - 1] = '\0';
}

int IsCapitalStart(const char* str) {
    if (!str || *str == '\0') return 0;
    return isupper((unsigned char)str[0]) != 0;
}

size_t CpStringLength(const char* str) {
    return strlen(str);
}

int CpStringContains(const char* str, const char* substr) {
    if (!str || !substr) return 0;
    return strstr(str, substr) != NULL;
}