#include "../../include/utility_shell.h"
#include "../../include/fat32.h"
#include "../../lib/lib-header/string.h"
#include "../../lib/lib-header/stdmem.h"

char* current_directory = "/";
uint32_t cwd_cluster_number = 2;
struct FAT32DirectoryTable cwd_table;

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

void ls() {
    syscall(7, (uint32_t) &cwd_table, cwd_cluster_number, 0);
    
    // Iterate through the directory table
    for (int32_t i = 1; i < (int32_t)(CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
        // If the entry is empty, skip
        if (cwd_table.table[i].name[0] == 0x00)
            continue;

        // If its a folder
        if (cwd_table.table[i].attribute == ATTR_SUBDIRECTORY) {
            puts(cwd_table.table[i].name, BIOS_GREEN);
            puts("/", BIOS_GREEN);
            puts("\n", BIOS_GRAY);
        } else {
            // If its a file
            puts(cwd_table.table[i].name, BIOS_GREEN);
            puts("\n", BIOS_GRAY);
        }
    }
}

void copy(char* src_name, char* src_ext, uint32_t src_parent_number, char* target_name, char* target_ext, uint32_t target_parent_number) {
    uint32_t src_size;
    bool is_dir = 0;
    struct FAT32DirectoryTable src_table;
    uint32_t src_cluster_number;
    struct FAT32DirectoryTable src_parent_table;
    syscall(7, (uint32_t)&src_parent_table, src_parent_number, 0);
    
    for (int32_t i = 0; i < (int32_t)(CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
        if (memcmp(src_parent_table.table[i].name, src_name, 8) == 0 &&
            memcmp(src_parent_table.table[i].ext, src_ext, 3) == 0) {
            src_size = src_parent_table.table[i].filesize;
            src_cluster_number = (src_parent_table.table[i].cluster_high << 16) | src_parent_table.table[i].cluster_low;
            is_dir = src_parent_table.table[i].attribute == ATTR_SUBDIRECTORY;
            if (is_dir)
                src_size = sizeof(struct FAT32DirectoryTable);
            break;
        }
    }
    
    struct ClusterBuffer data_buf[(src_size + CLUSTER_SIZE - 1) / CLUSTER_SIZE];
    struct FAT32DriverRequest request = {
        .buf = &data_buf,
        .name = "\0\0\0\0\0\0\0",
        .ext = "\0\0",
        .buffer_size = src_size,
        .parent_cluster_number = src_parent_number
    };
    memcpy(request.name, src_name, 8);
    memcpy(request.ext, src_ext, 3);

    int8_t retcode;
    if (is_dir) {
        request.buf = &src_table;
        syscall(1, (uint32_t)&request, (uint32_t)&retcode, 0);
    } else
        syscall(0, (uint32_t)&request, (uint32_t)&retcode, 0);

    if (is_dir) {
        memcpy(request.name, target_name, 8);
        memcpy(request.ext, target_ext, 3);
        request.parent_cluster_number = target_parent_number;
        request.buffer_size = 0;
        syscall(2, (uint32_t)&request, (uint32_t)&retcode, 0);

        uint32_t target_cluster_number;
        struct FAT32DirectoryTable target_parent_table;

        syscall(7, (uint32_t)&target_parent_table, target_parent_number, 0);

        for (int32_t i = 0; i < (int32_t)(CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
            if (memcmp(target_parent_table.table[i].name, target_name, 8) == 0 &&
                memcmp(target_parent_table.table[i].ext, target_ext, 3) == 0 &&
                target_parent_table.table[i].user_attribute == UATTR_NOT_EMPTY) {
                target_cluster_number = (target_parent_table.table[i].cluster_high << 16) | target_parent_table.table[i].cluster_low;
            }
        }

        for (int32_t i = 1; i < (int32_t)(CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
            if (src_table.table[i].user_attribute == UATTR_NOT_EMPTY) {
                copy(src_table.table[i].name, src_table.table[i].ext, src_cluster_number, src_table.table[i].name, src_table.table[i].ext, target_cluster_number);
            }
        }
    } else {
        memcpy(request.name, target_name, 8);
        memcpy(request.ext, target_ext, 3);
        request.parent_cluster_number = target_parent_number;
        syscall(2, (uint32_t)&request, (uint32_t)&retcode, 0);
        if (retcode != 0)
            puts("Error writing to file", BIOS_GRAY);
    }
}

void cp(char* command) {
    uint16_t n_words = countWords(command);
    int16_t recursive = -1;
    int8_t retcode = 0;

    struct FAT32DirectoryTable table_buf = {0};
    struct FAT32DriverRequest request = {
        .buf = &table_buf,
        .name = "\0\0\0\0\0\0\0",
        .ext = "\0\0",
        .parent_cluster_number = cwd_cluster_number,
        .buffer_size = sizeof(struct FAT32DirectoryEntry)
    };

    for (uint16_t i = 1; i < n_words; i++) {
        uint16_t n = wordLen(command, i);
        char word[n + 1];
        getWord(command, i, word);
        if (strcmp(word, "-r"))
            recursive = i;
    }

    if ((recursive == -1 && n_words < 3) || (recursive != -1 && n_words < 4)) {
        puts(": missing file operands\n", BIOS_GRAY);
        return;
    }

    uint16_t target_idx;
    if (recursive == n_words - 1)
        target_idx = n_words - 2;
    else
        target_idx = n_words - 1;

    // check if all source exists
    for (uint16_t i = 1; i < n_words; i++) {
        if (recursive == i || target_idx == i) continue;
        char filename[12];
        getWord(command, i, filename);

        char name[9];
        char ext[4];

        // if filename is too long
        if (parseFileName(filename, name, ext)) {
            puts(filename, BIOS_GRAY);
            puts(": filename invalid, name or extension may be too long\n", BIOS_GRAY);
            return;
        }

        memcpy(request.name, name, 8);
        memcpy(request.ext, ext, 3);
        syscall(1, (uint32_t)&request, (uint32_t)&retcode, 0);
        if (retcode == 2) {
            puts(filename, BIOS_GRAY);
            puts(": file not found\n", BIOS_GRAY);
            return;
        }
        if (retcode == 0 && recursive == -1) {
            puts(filename, BIOS_GRAY);
            puts(": is a directory;  -r not specified\n", BIOS_GRAY);
            return;
        }
    }

    uint16_t target_n = wordLen(command, target_idx);
    char target_filename[target_n + 1];
    getWord(command, target_idx, target_filename);

    char target_name[9];
    char target_ext[4];

    if (parseFileName(target_filename, target_name, target_ext)) {
        puts(target_filename, BIOS_GRAY);
        puts(": filename invalid, name or extension may be too long\n", BIOS_GRAY);
        return;
    }

    memcpy(request.name, target_name, 8);
    memcpy(request.ext, target_ext, 3);
    syscall(1, (uint32_t)&request, (uint32_t)&retcode, 0);
    
    // target is an existing directory
    if (retcode == 0) {
        uint32_t target_cluster_number;
        for (int32_t i = 0; i < (int32_t)(CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
            if (memcmp(cwd_table.table[i].name, target_name, 8) == 0 &&
                memcmp(cwd_table.table[i].ext, target_ext, 3) == 0) {
                target_cluster_number = (cwd_table.table[i].cluster_high << 16) | cwd_table.table[i].cluster_low;
            }
        }

        for (int16_t i = 1; i < n_words; i++) {
            if (i == recursive || i == target_idx) continue;
            char filename[12];
            getWord(command, i, filename);
            char name[9];
            char ext[4];
            parseFileName(filename, name, ext);

            copy(name, ext, cwd_cluster_number, name, ext, target_cluster_number);
        }
    } else if (retcode == 1 || retcode == 2) {
        if ((recursive == -1 && n_words > 3) || (recursive != -1 && n_words > 4)) {
            puts(target_filename, BIOS_GRAY);
            puts(": is not a folder\n", BIOS_GRAY);
            return;
        } else {
            if (retcode == 1)
                syscall(3, (uint32_t)&request, (uint32_t)&retcode, 0);

            for (int16_t i = 1; i < n_words; i++) {
                if (i == recursive || i == target_idx) continue;
                char filename[12];
                getWord(command, i, filename);
                char name[9];
                char ext[4];
                parseFileName(filename, name, ext);

                copy(name, ext, cwd_cluster_number, target_name, target_ext, cwd_cluster_number);
            }
        }
    }
}

void remove(char* name, char* ext, uint32_t parent_number) {
    int8_t retcode;

    struct ClusterBuffer data_buf;
    struct FAT32DriverRequest request = {
        .buf = &data_buf,
        .name = "\0\0\0\0\0\0\0",
        .ext = "\0\0",
        .buffer_size = 0,
        .parent_cluster_number = parent_number
    };
    memcpy(request.name, name, 8);
    memcpy(request.ext, ext, 3);
    syscall(3, (uint32_t)&request, (uint32_t)&retcode, 0);

    if (retcode == 2) {
        uint32_t target_cluster_number;
        struct FAT32DirectoryTable parent_table;
        syscall(7, (uint32_t)&parent_table, parent_number, 0);

        for (int32_t i = 0; i < (int32_t)(CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
            if (memcmp(parent_table.table[i].name, name, 8) == 0 &&
                memcmp(parent_table.table[i].ext, ext, 3) == 0 &&
                parent_table.table[i].user_attribute == UATTR_NOT_EMPTY) {
                target_cluster_number = (parent_table.table[i].cluster_high << 16) | parent_table.table[i].cluster_low;
            }
        }

        struct FAT32DirectoryTable target_table;
        request.buf = &target_table;
        request.buffer_size = sizeof(struct FAT32DirectoryTable);
        syscall(1, (uint32_t)&request, (uint32_t)&retcode, 0);
        
        for (int32_t i = 1; i < (int32_t)(CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
            if (target_table.table[i].user_attribute == UATTR_NOT_EMPTY) {
                remove(target_table.table[i].name, target_table.table[i].ext, target_cluster_number);
            }
        }
    }
    memcpy(request.name, name, 8);
    memcpy(request.ext, ext, 3);
    request.parent_cluster_number = parent_number;
    syscall(3, (uint32_t)&request, (uint32_t)&retcode, 0);
}

void rm(char* command) {
    uint16_t n_words = countWords(command);
    int16_t recursive = -1;
    int8_t retcode = 0;

    struct FAT32DirectoryTable table_buf = {0};
    struct FAT32DriverRequest request = {
        .buf = &table_buf,
        .name = "\0\0\0\0\0\0\0",
        .ext = "\0\0",
        .parent_cluster_number = cwd_cluster_number,
        .buffer_size = sizeof(struct FAT32DirectoryEntry)
    };

    for (uint16_t i = 1; i < n_words; i++) {
        uint16_t n = wordLen(command, i);
        char word[n + 1];
        getWord(command, i, word);
        if (strcmp(word, "-r"))
            recursive = i;
    }

    if ((recursive == -1 && n_words < 2) || (recursive != -1 && n_words < 3)) {
        puts(": missing file operands\n", BIOS_GRAY);
        return;
    }

    // check if all files exists
    for (uint16_t i = 1; i < n_words; i++) {
        if (recursive == i) continue;
        char filename[12];
        getWord(command, i, filename);

        char name[9];
        char ext[4];

        // if filename is too long
        if (parseFileName(filename, name, ext)) {
            puts(filename, BIOS_GRAY);
            puts(": filename invalid, name or extension may be too long\n", BIOS_GRAY);
            return;
        }

        memcpy(request.name, name, 8);
        memcpy(request.ext, ext, 3);
        syscall(1, (uint32_t)&request, (uint32_t)&retcode, 0);
        if (retcode == 2) {
            puts(filename, BIOS_GRAY);
            puts(": file not found\n", BIOS_GRAY);
            return;
        }
        if (retcode == 0 && recursive == -1) {
            puts(filename, BIOS_GRAY);
            puts(": is a directory;  -r not specified\n", BIOS_GRAY);
            return;
        }
    }

    for (uint16_t i = 1; i < n_words; i++) {
        if (recursive == i) continue;
        char filename[12];
        getWord(command, i, filename);

        char name[9];
        char ext[4];

        // if filename is too long
        if (parseFileName(filename, name, ext)) {
            puts(filename, BIOS_GRAY);
            puts(": filename invalid, name or extension may be too long\n", BIOS_GRAY);
            return;
        }

        remove(name, ext, cwd_cluster_number);
    }
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
        ls();
    }
    else if (strcmp(command, "mkdir")) {
        // panggil fungsi
    }
    else if (strcmp(command, "cat")) {
        // panggil fungsi
    }
    else if (strcmp(command, "cp")) {
        cp(buf);
    }
    else if (strcmp(command, "rm")) {
        rm(buf);
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