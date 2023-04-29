/* Custom string library */
#ifndef STRING_H
#define STRING_H

#include "stdtype.h"

#define NULL ((void*)0)

int strlen(const char *str);

// 1 if equal
int strcmp(const char *str1, const char *str2);

int strncmp(const char *str1, const char *str2, uint16_t n);

char *strtok(char *str, const char *delim);

void addTrailingNull(char *str, uint16_t start, uint16_t end);

// 0 if success, 1 if filename too long
int parseFileName(char *filename, char *name, char *ext);

int countWords(const char* str);

int wordLen(const char* str, uint16_t idx);

void getWord(const char* str, uint16_t idx, char* buf);

/**
 * Parses a string into a series of tokens based on the
 * specified delimiter string. It maintains a static pointer to the current
 * position in the string, allowing subsequent calls to parse additional tokens
 * from the same string. The delimiter string may contain multiple delimiters,
 * each of which may be a single character.
 *
 * @param str The string to parse.
 * @param delim The delimiter string.
 *
 * @return A pointer to the next token in the string, or NULL if there are no
 *         more tokens.
 */
char* custom_strtok(char* str, const char* delim);

#endif