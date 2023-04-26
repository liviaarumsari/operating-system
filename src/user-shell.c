#include "../lib/lib-header/stdtype.h"
#include "../lib/lib-header/string.h"
#include "../include/fat32.h"
#include "../include/utility_shell.h"

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

int main(void) {
    char buf[16];
    while (TRUE) {
        puts("sOS@OS-IF2230", BIOS_GREEN);
        puts(":", BIOS_GRAY);
        puts(current_directory, BIOS_BLUE);
        puts("$", BIOS_GRAY);
        syscall(4, (uint32_t) buf, 16, 0);
    }

    return 0;
}
