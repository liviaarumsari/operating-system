#include "../../include/utility_shell.h"
#include "../../include/fat32.h"
#include "../../lib/lib-header/string.h"
#include "../../lib/lib-header/stdmem.h"

char* current_directory = "/";
uint32_t cwd_cluster_number = 2;

void syscall(uint32_t eax, uint32_t ebx, uint32_t ecx, uint32_t edx) {
    __asm__ volatile("mov %0, %%ebx" : /* <Empty> */ : "r"(ebx));
    __asm__ volatile("mov %0, %%ecx" : /* <Empty> */ : "r"(ecx));
    __asm__ volatile("mov %0, %%edx" : /* <Empty> */ : "r"(edx));
    __asm__ volatile("mov %0, %%eax" : /* <Empty> */ : "r"(eax));
    // Note : gcc usually use %eax as intermediate register,
    //        so it need to be the last one to mov
    __asm__ volatile("int $0x30");
}

void puts(char* buf, uint8_t color) {
    syscall(5, (uint32_t) buf, strlen(buf), color);
}

void executeCommand(char* buf) {
    uint16_t n = wordLen(buf, 0);
    char command[n + 1];
    getWord(buf, 0, command);

    if (strcmp(command, "cd")) {
        // panggil fungsi
    }
    else if (strcmp(command, "ls")) {
        // panggil fungsi
    }
    else if (strcmp(command, "mkdir")) {
        // panggil fungsi
    }
    else if (strcmp(command, "cat")) {
        // panggil fungsi
    }
    else if (strcmp(command, "cp")) {
        // panggil fungsi
    }
    else if (strcmp(command, "rm")) {
        // panggil fungsi
    }
    else if (strcmp(command, "mv")) {
        // panggil fungsi
    }
    else if (strcmp(command, "whereis")) {
        whereisCommand(buf);
    }
    else if (strcmp(command, "clear")) {
        syscall(6, 0, 0, 0);
    }
    else {
        puts(command, BIOS_GRAY);
        puts(": command not found\n", BIOS_GRAY);
    }
}

void whereisCommand(char* buf) {
    uint16_t n = wordLen(buf, 1);
    char searchName[n + 1];
    getWord(buf, 1, searchName);
    
    char name[8];
    char ext[3];

    // if filename is too long
    if (parseFileName(searchName, name, ext)) {
        puts(searchName, BIOS_GRAY);
        puts(": filename invalid, name or extension may be too long\n", BIOS_GRAY);
        return;
    }

    DFSsearch("root\0\0\0", ROOT_CLUSTER_NUMBER, name);
}

void DFSsearch(char* folderName, uint32_t parentCluster, char* searchName) {
    struct FAT32DirectoryTable dirtable;
    struct FAT32DriverRequest request = {
        .buf                   = &dirtable,
        .name                  = "",
        .ext                   = "\0\0\0",
        .parent_cluster_number = parentCluster,
        .buffer_size           = CLUSTER_SIZE
    };
    memcpy(request.name, folderName, 8);
    int32_t retcode;
    syscall(1, (uint32_t) &request, (uint32_t) &retcode, 0);
    if (retcode == 0) {
        size_t i = 1;
        if (strcmp(searchName, "root\0\0\0") && parentCluster == 0x2) {
            i = 0;
        }
        for (; i < CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry); i++) {
            uint32_t clusterAddress = (dirtable.table[i].cluster_high << 16) | dirtable.table[i].cluster_low;
            if (strcmp(dirtable.table[i].name,searchName)) {
                puts("root", BIOS_GRAY);
                constructPath(clusterAddress);
                if (dirtable.table[i].attribute != ATTR_SUBDIRECTORY && !strcmp(dirtable.table[i].ext, "\0\0\0")) {
                    puts(".", BIOS_GRAY);
                    puts(dirtable.table[i].ext, BIOS_GRAY);
                }
            }
            if (dirtable.table[i].attribute == ATTR_SUBDIRECTORY) {
                DFSsearch(dirtable.table[i].name, clusterAddress, searchName);
            }
        }  
    }
}

void constructPath(uint32_t clusterAddress) {
    struct FAT32DirectoryTable dirtable;
    syscall(7, (uint32_t) &dirtable, clusterAddress, 0);
    if (!strcmp(dirtable.table[0].name, "root\0\0\0")) {
        uint32_t parentCluster = (dirtable.table[0].cluster_high << 16) | dirtable.table[0].cluster_low;
        constructPath(parentCluster);
        puts("/", BIOS_GRAY);
        puts(dirtable.table[0].name, BIOS_GRAY);
    }
    
}