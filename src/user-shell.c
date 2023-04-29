#include "../lib/lib-header/stdtype.h"
#include "../include/fat32.h"
#include "../include/utility_shell.h"

int main(void) {
    char buf[256];

    while (TRUE) {
        syscall(7, (uint32_t)&cwd_table, (uint32_t)cwd_cluster_number, 0);

        puts("sOS@OS-IF2230", BIOS_GREEN);
        puts(":", BIOS_GRAY);
        puts(current_directory, BIOS_BLUE);
        puts("$", BIOS_GRAY);
        syscall(4, (uint32_t) buf, 256, 0);
        executeCommand(buf);
    }
    return 0;
}
