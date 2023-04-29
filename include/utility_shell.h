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
 * Parses a path into individual parts and stores them
 * in an array of strings. The parts are separated by forward slashes ('/').
 * Any consecutive forward slashes in the path are treated as a single delimiter.
 * The number of parts found is stored in the provided integer pointer.
 *
 * @param path The path to parse.
 * @param parts An array of strings to store the parts in.
 * @param numParts A pointer to an integer to store the number of parts found.
 */
void parsePath(const char* path, char** parts, int* numParts);

/**
 * Implementation of the `mkdir` command
*/
void mkdir(char* dirname);

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

void DFSsearch(char* folderName, uint32_t parentCluster, char* searchName);

void constructPath(uint32_t clusterAddress);

#endif