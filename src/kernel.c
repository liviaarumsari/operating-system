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
    pic_remap();
    initialize_idt();
    activate_keyboard_interrupt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    initialize_filesystem_fat32();
    keyboard_state_activate();

    struct ClusterBuffer cbuf[5];
    for (uint32_t i = 0; i < 5; i++)
        for (uint32_t j = 0; j < CLUSTER_SIZE; j++)
            cbuf[i].buf[j] = i + 'a';

    struct FAT32DriverRequest request = {
        .buf                   = cbuf,
        .name                  = "ikanaide",
        .ext                   = "\0\0\0",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = 0,
    } ;

    request.buffer_size = CLUSTER_SIZE;
    write(request);  // Create fragmented file "daijoubu"

    while (TRUE);
}
