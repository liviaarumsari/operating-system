#include "../lib/lib-header/portio.h"
#include "../lib/lib-header/stdtype.h"
#include "../lib/lib-header/stdmem.h"
#include "../include/gdt.h"
#include "../include/framebuffer.h"
#include "../include/kernel_loader.h"
#include "../include/idt.h"
#include "../include/interrupt.h"
#include "../include/fat32.h"
#include "../include/keyboard.h"
#include "../include/paging.h"

void kernel_setup(void) {
    enter_protected_mode(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    initialize_filesystem_fat32();
    gdt_install_tss();
    set_tss_register();

    // Allocate first 4 MiB virtual memory
    allocate_single_user_page_frame((uint8_t*) 0);

    // Write shell into memory (assuming shell is less than 1 MiB)
    struct FAT32DriverRequest request = {
        .buf                   = (uint8_t*) 0,
        .name                  = "shell",
        .ext                   = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = 0x100000,
    };
    read(request);

     struct ClusterBuffer single_clusterx;
    for (uint32_t j = 0; j < CLUSTER_SIZE; j++)
        single_clusterx.buf[j] = 'x';

    struct FAT32DriverRequest request_folder1 = {
        .buf                   = &single_clusterx,
        .name                  = "folder1\0",
        .ext                   = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = 0,
    } ;

    write(request_folder1);

    struct FAT32DriverRequest request_file1 = {
        .buf                   = &single_clusterx,
        .name                  = "file1\0\0\0",
        .ext                   = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = CLUSTER_SIZE,
    } ;

    write(request_file1);

    struct ClusterBuffer single_clustera;
    for (uint32_t j = 0; j < CLUSTER_SIZE; j++)
        single_clustera.buf[j] = 'a';

    struct FAT32DriverRequest request_file2 = {
        .buf                   = &single_clustera,
        .name                  = "file2\0\0\0",
        .ext                   = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = CLUSTER_SIZE,
    };

    write(request_file2);

    struct FAT32DriverRequest request_file3 = {
        .buf                   = &single_clustera,
        .name                  = "file3\0\0\0",
        .ext                   = "\0\0\0",
        .parent_cluster_number = 9,
        .buffer_size           = CLUSTER_SIZE,
    };

    write(request_file3);

    struct FAT32DriverRequest request_folder2 = {
        .buf                   = &single_clustera,
        .name                  = "folder2\0",
        .ext                   = "\0\0\0",
        .parent_cluster_number = 9,
        .buffer_size           = 0,
    };

    write(request_folder2);

    struct FAT32DriverRequest request_file4 = {
        .buf                   = &single_clustera,
        .name                  = "file4\0\0\0",
        .ext                   = "\0\0\0",
        .parent_cluster_number = 13,
        .buffer_size           = CLUSTER_SIZE,
    };

    write(request_file4);

    // Set TSS $esp pointer and jump into shell 
    set_tss_kernel_current_stack();
    kernel_execute_user_program((uint8_t *) 0);

    while (TRUE);
}
