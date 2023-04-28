#ifndef UTILITY_SHELL_H
#define UTILITY_SHELL_H

#include "../lib/lib-header/stdtype.h"

#define BIOS_BLUE 0x01
#define BIOS_GREEN 0x02
#define BIOS_CYAN 0x03
#define BIOS_GRAY 0x07

extern char* current_directory;
extern uint32_t cwd_cluster_number;

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

void puts(char* buf, uint8_t color);

void executeCommand(char* buf);

void whereisCommand(char* buf);

void DFSsearch(char* folderName, uint32_t parentCluster, char* searchName, int8_t* isFound);

void constructPath(uint32_t clusterAddress);

#endif