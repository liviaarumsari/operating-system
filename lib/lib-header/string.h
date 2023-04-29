/* Custom string library */
#ifndef STRING_H
#define STRING_H

#include "stdtype.h"

int strlen(const char *str);

// 1 if equal
int strcmp(const char *str1, const char *str2);

void addTrailingNull(char *str, uint16_t start, uint16_t end);

// 0 if success, 1 if filename too long
int parseFileName(char *filename, char *name, char *ext);

int countWords(const char* str);

int wordLen(const char* str, uint16_t idx);

void getWord(const char* str, uint16_t idx, char* buf);

#endif