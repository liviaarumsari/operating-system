#include "../../include/utility_shell.h"
#include "../../lib/lib-header/string.h"

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
    char command[n];
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
        // panggil fungsi
    }
    else if (strcmp(command, "clear")) {
        syscall(6, 0, 0, 0);
    }
    else {
        puts(command, BIOS_GRAY);
        puts(": command not found\n", BIOS_GRAY);
    }
}