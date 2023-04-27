#include "../lib/lib-header/stdtype.h"
#include "../include/fat32.h"
#include "../include/utility_shell.h"

int main(void) {
    char buf[256];
    while (TRUE) {
        puts("sOS@OS-IF2230", BIOS_GREEN);
        puts(":", BIOS_GRAY);
        puts(current_directory, BIOS_BLUE);
        puts("$", BIOS_GRAY);
        syscall(4, (uint32_t) buf, 16, 0);
        executeCommand(buf);
    }
    return 0;
}
