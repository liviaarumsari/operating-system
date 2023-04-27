#ifndef UTILITY_SHELL_H
#define UTILITY_SHELL_H

#include "../lib/lib-header/stdtype.h"

#define BIOS_BLUE 0x01
#define BIOS_GREEN 0x02
#define BIOS_CYAN 0x03
#define BIOS_GRAY 0x07

extern char* current_directory;

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

void puts(char* buf, uint8_t color);

void executeCommand(char* buf);

#endif