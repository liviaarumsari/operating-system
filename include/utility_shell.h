#ifndef UTILITY_SHELL_H
#define UTILITY_SHELL_H

#include "../lib/lib-header/stdtype.h"
#include "../lib/lib-header/stdmem.h"
#include "fat32.h"

#define BIOS_BLUE 0x01
#define BIOS_GREEN 0x02
#define BIOS_CYAN 0x03
#define BIOS_GRAY 0x07
#define BIOS_RED 0x0C

extern char* current_directory;
extern uint32_t cwd_cluster_number;
extern struct FAT32DirectoryTable cwd_table;

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx);

void puts(char* buf, uint8_t color);

void copy(char* src_name, char* src_ext, uint32_t src_parent_number, char* target_name, char* target_ext, uint32_t target_parent_number);

/**
 * If source has folder but no -r flag: ERROR
 * If target exists:
 *  - target_directory: copy all source into target_directory
 *  - target_file:
 *      - single source: overwrite
 *      - multiple source: ERROR
 * If target does not exist:
 *  - If multiple source: ERROR
 *  - If single source:
 *      - If source_directory: create new directory
 *      - If source_file: create new file
*/
void cp(char* command);

void remove(char* name, char* ext, uint32_t parent_number);

void rm(char* command);

void executeCommand(char* buf);

void whereisCommand(char* buf);

void DFSsearch(uint32_t folderAddress, char* searchName, int8_t* isFound);

void constructPath(uint32_t clusterAddress);

void moveCommand(char* buf);

#endif