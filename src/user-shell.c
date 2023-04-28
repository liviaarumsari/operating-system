#include "../lib/lib-header/stdtype.h"
#include "../include/fat32.h"
#include "../include/utility_shell.h"

int main(void) {
    char buf[256];

    struct FAT32DriverRequest req = {
        .buf = &cwd_table,
        .name = "root\0\0\0",
        .ext = "\0\0\0",
        .buffer_size = sizeof(struct FAT32DirectoryTable),
        .parent_cluster_number = 2
    };
    int retcode;
    syscall(1, (uint32_t)&req, &retcode, 0);

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
