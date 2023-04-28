#include "../lib-header/string.h"

int strlen(const char *str)
{
    int len = 0;
    while (*str != '\0')
    {
        len++;
        str++;
    }
    return len;
}

int strcmp(const char *str1, const char *str2) {
    uint16_t i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {
        if (str1[i] != str2[i])
            return 0;
        i++;
    }
    if (str1[i] != str2[i])
        return 0;
    return 1;
}

int countWords(const char* str) {
    int ctr = 0;
    for (int i = 0; i<strlen(str)-1; i++) {
        if (str[0] != ' ') {
            ctr++;
        }
        else if (str[i] ==' ' && str[i+1] != ' ') {
            ctr++;
        }
    }
    return ctr;
}

int wordLen(const char* str, uint16_t idx) {
    int ctr = 0;
    int word_start = -1;
    int word_end = -1;

    for (int i = 0; i<strlen(str)-1; i++) {
        if (word_start != -1 && str[i] == ' ') {
            word_end = i;
        }
        if (str[0] != ' ') {
            ctr++;
            if (ctr == idx + 1) {
                word_start = i;
            }
        }
        else if (str[i] ==' ' && str[i+1] != ' ') {
            ctr++;
            if (ctr == idx + 1) {
                word_start = i + 1;
            }
        }
    }

    if (word_end == -1)
        word_end = strlen(str);

    uint16_t n = word_end - word_start;
    return n;
}

void getWord(const char* str, uint16_t idx, char* buf) {
    int ctr = 0;
    int word_start = -1;

    for (int i = 0; i<strlen(str)-1; i++) {
        if (str[0] != ' ') {
            ctr++;
            if (ctr == idx + 1) {
                word_start = i;
            }
        }
        else if (str[i] ==' ' && str[i+1] != ' ') {
            ctr++;
            if (ctr == idx + 1) {
                word_start = i + 1;
            }
        }
    }
    
    uint16_t n = wordLen(str, idx);
    for (int i = 0; i < n; i++) {
        buf[i] = str[word_start + i];
    }
    buf[n] = '\0';
}