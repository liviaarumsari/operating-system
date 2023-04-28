/* Custom string library */
#ifndef STRING_H
#define STRING_H

#include "stdtype.h"

int strlen(const char *str);

// 1 if equal
int strcmp(const char *str1, const char *str2);

int countWords(const char* str);

int wordLen(const char* str, uint16_t idx);

void getWord(const char* str, uint16_t idx, char* buf);

#endif