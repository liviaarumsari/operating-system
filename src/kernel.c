#include "../lib/lib-header/portio.h"
#include "../lib/lib-header/stdtype.h"
#include "../lib/lib-header/stdmem.h"
#include "../include/gdt.h"
#include "../include/framebuffer.h"
#include "../include/kernel_loader.h"
#include "../include/idt.h"
#include "../include/interrupt.h"
#include "../include/fat32.h"

void kernel_setup(void) {
    enter_protected_mode(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    initialize_filesystem_fat32();

    // struct ClusterBuffer cbuf[5];
    // for (uint32_t i = 0; i < 5; i++)
    //     for (uint32_t j = 0; j < CLUSTER_SIZE; j++)
    //         cbuf[i].buf[j] = i + 'a';

    // struct ClusterBuffer control_buf[5];
    // for (uint32_t i = 0; i < 5; i++)
    //     for (uint32_t j = 0; j < CLUSTER_SIZE; j++)
    //         control_buf[i].buf[j] = i + 'v';

    struct ClusterBuffer single_cluster;
    for (uint32_t j = 0; j < CLUSTER_SIZE; j++)
        single_cluster.buf[j] = 'x';

    struct FAT32DriverRequest request = {
        .buf                   = &single_cluster,
        .name                  = "ikanaide",
        .ext                   = "uwu",
        .parent_cluster_number = ROOT_CLUSTER_NUMBER,
        .buffer_size           = CLUSTER_SIZE,
    } ;

    for (size_t i = 0; i < 129; i++)
    {
        request.name[0] = i % 26 + 'a';
        request.name[1] = i / 26 + 'a';
        request.name[2] = '\0';
        request.name[3] = '\0';
        request.name[4] = '\0';
        request.name[5] = '\0';
        request.name[6] = '\0';
        request.name[7] = '\0';

        write(request);
    }
    

    // write(request);  // Create folder "ikanaide"
    // memcpy(request.name, "kano1\0\0\0", 8);
    // write(request);  // Create folder "kano1"
    // memcpy(request.name, "ikanaide", 8);
    // memcpy(request.ext, "\0\0\0", 3);
    // delete(request); // Delete first folder, thus creating hole in FS

    // memcpy(request.name, "daijoubu", 8);
    // request.buffer_size = 5*CLUSTER_SIZE;
    // write(request);  // Create fragmented file "daijoubu"

    // struct ClusterBuffer readcbuf;
    // read_clusters(&readcbuf, ROOT_CLUSTER_NUMBER+1, 1); 
    // // If read properly, readcbuf should filled with 'a'

    // request.buf = control_buf;
    // request.buffer_size = CLUSTER_SIZE;
    // read(request);   // Failed read due not enough buffer size
    // request.buffer_size = 5*CLUSTER_SIZE;
    // read(request);   // Success read on file "daijoubu"

    while (TRUE);
}
