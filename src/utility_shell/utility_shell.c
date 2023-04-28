#include "../../include/utility_shell.h"
#include "../../lib/lib-header/string.h"

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

void copy(char* src_name, char* src_ext, uint32_t src_parent_number, char* target_name, char* target_ext, uint32_t target_parent_number) {
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

    // check if all source exists
    for (uint16_t i = 1; i < n_words; i++) {
        uint16_t n = wordLen(command, i);
        char filename[n + 1];
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
        if (retcode == 0 && recursive != -1) {
            puts(filename, BIOS_GRAY);
            puts(": is a directory;  -r not specified\n", BIOS_GRAY);
            return;
        }
    }

    uint16_t target_idx;
    if (recursive == n_words - 1)
        target_idx = n_words - 2;
    else
        target_idx = n_words - 1;
    uint16_t target_n = wordLen(command, target_idx);
    char target_filename[target_n + 1];
    getWord(command, target_idx, target_filename);

    char target_name[9];
    char target_ext[4];
    memcpy(request.name, target_name, 8);
    memcpy(request.ext, target_ext, 3);
    syscall(1, (uint32_t)&request, (uint32_t)&retcode, 0);
    
    // target exists
    if (retcode != 2) {
        // target is a directory
        if (retcode == 0) {
            uint32_t target_cluster_number;
            for (int32_t i = 0; i < (int32_t)(CLUSTER_SIZE / sizeof(struct FAT32DirectoryEntry)); i++) {
                if (memcmp(cwd_table.table[i].name, target_name, 8) == 0 &&
                    memcmp(cwd_table.table[i].ext, target_ext, 3) == 0) {
                    target_cluster_number = (cwd_table.table[i].cluster_high << 16) | cwd_table.table[i].cluster_low;
                }
            }

            for (int16_t i = 1; i < n_words; i++) {
                if (i == recursive) continue;
                uint16_t n = wordLen(command, i);
                char filename[n + 1];
                getWord(command, i, filename);
                char name[9];
                char ext[4];

                copy(name, ext, cwd_cluster_number, name, ext, target_cluster_number);
            }
        }
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
        // panggil fungsi
    }
    else if (strcmp(command, "mv")) {
        // panggil fungsi
    }
    else if (strcmp(command, "whereis")) {
        // panggil fungsi
    }
    else if (strcmp(command, "clear")) {
        // panggil fungsi
    }
    else {
        puts("Command not found", BIOS_GRAY);
    }
}