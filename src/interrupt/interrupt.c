#include "../../include/interrupt.h"

#include "../../include/fat32.h"
#include "../../include/framebuffer.h"
#include "../../include/idt.h"
#include "../../include/keyboard.h"
#include "../../lib/lib-header/portio.h"
#include "../../lib/lib-header/stdmem.h"

struct TSSEntry _interrupt_tss_entry = {
    .ss0 = GDT_KERNEL_DATA_SEGMENT_SELECTOR,
};

void io_wait(void) {
    out(0x80, 0);
}

void pic_ack(uint8_t irq) {
    if (irq >= 8)
        out(PIC2_COMMAND, PIC_ACK);
    out(PIC1_COMMAND, PIC_ACK);
}

void pic_remap(void) {
    uint8_t a1, a2;

    // Save masks
    a1 = in(PIC1_DATA);
    a2 = in(PIC2_DATA);

    // Starts the initialization sequence in cascade mode
    out(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    out(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    out(PIC1_DATA, PIC1_OFFSET);  // ICW2: Master PIC vector offset
    io_wait();
    out(PIC2_DATA, PIC2_OFFSET);  // ICW2: Slave PIC vector offset
    io_wait();
    out(PIC1_DATA, 0b0100);  // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    io_wait();
    out(PIC2_DATA, 0b0010);  // ICW3: tell Slave PIC its cascade identity (0000 0010)
    io_wait();

    out(PIC1_DATA, ICW4_8086);
    io_wait();
    out(PIC2_DATA, ICW4_8086);
    io_wait();

    // Restore masks
    out(PIC1_DATA, a1);
    out(PIC2_DATA, a2);
}

void main_interrupt_handler(__attribute__((unused)) struct CPURegister cpu, uint32_t int_number,
                            __attribute__((unused)) struct InterruptStack info) {
    switch (int_number) {
        case (PIC1_OFFSET + IRQ_KEYBOARD):
            keyboard_isr();
            break;
        case 0x30:
            syscall(cpu, info);
            break;
    }
}

void activate_keyboard_interrupt(void) {
    out(PIC1_DATA, PIC_DISABLE_ALL_MASK ^ (1 << IRQ_KEYBOARD));
    out(PIC2_DATA, PIC_DISABLE_ALL_MASK);
}

void set_tss_kernel_current_stack(void) {
    uint32_t stack_ptr;
    // Reading base stack frame instead esp
    __asm__ volatile("mov %%ebp, %0" : "=r"(stack_ptr) : /* <Empty> */);
    // Add 8 because 4 for ret address and other 4 is for stack_ptr variable
    _interrupt_tss_entry.esp0 = stack_ptr + 8;
}

void syscall(struct CPURegister cpu, __attribute__((unused)) struct InterruptStack info) {
    if (cpu.eax == 0) {
        struct FAT32DriverRequest request = *(struct FAT32DriverRequest*)cpu.ebx;
        *((int8_t*)cpu.ecx) = read(request);
    } else if (cpu.eax == 1) {
        struct FAT32DriverRequest request = *(struct FAT32DriverRequest*)cpu.ebx;
        *((int8_t*)cpu.ecx) = read_directory(request);
    } else if (cpu.eax == 2) {
        struct FAT32DriverRequest request = *(struct FAT32DriverRequest*)cpu.ebx;
        *((int8_t*)cpu.ecx) = write(request);
    } else if (cpu.eax == 3) {
        struct FAT32DriverRequest request = *(struct FAT32DriverRequest*)cpu.ebx;
        *((int8_t*)cpu.ecx) = delete (request);
    } else if (cpu.eax == 4) {
        keyboard_state_activate();
        __asm__("sti");  // Due IRQ is disabled when main_interrupt_handler() called
        while (is_keyboard_blocking())
            ;
        char buf[KEYBOARD_BUFFER_SIZE];
        get_keyboard_buffer(buf);
        memcpy((char*)cpu.ebx, buf, cpu.ecx);
    } else if (cpu.eax == 5) {
        char* str = (char*)cpu.ebx;
        uint32_t len = cpu.ecx;
        uint8_t fg = cpu.edx & 0xFF;         // extract foreground color from edx
        uint8_t bg = (cpu.edx >> 8) & 0xFF;  // extract background color from edx

        for (uint32_t i = 0; i < len; i++) {
            if (str[i] == '\n') {
                uint32_t temp = BUFFER_COUNT + (80 - (BUFFER_COUNT % 80));
                BUFFER_COUNT = temp;
            } else {
                framebuffer_write(BUFFER_COUNT / 80, BUFFER_COUNT % 80, str[i], fg, bg);
                BUFFER_COUNT++;
                if (BUFFER_COUNT % 80 == 79)
                    BUFFER_COUNT = BUFFER_COUNT + (80 - (BUFFER_COUNT % 80));
            }
            framebuffer_set_cursor(BUFFER_COUNT / 80, BUFFER_COUNT % 80);
        }
    } else if (cpu.eax == 6) {
        framebuffer_clear();
        BUFFER_COUNT = 0;
        framebuffer_set_cursor(0, 0);
    } else if (cpu.eax == 7) {
        struct FAT32DirectoryTable *dirtable = (struct FAT32DirectoryTable*)cpu.ebx;
        read_clusters(dirtable, cpu.ecx, 1);
    } else if (cpu.eax == 8) {
        // CPU.EBX: pointer to directory entry
        // CPU.ECX: 11 char array with first 8 chars being filename & last three being ext
        // CPU.EDX: parent cluster number
        char *name = (char *)cpu.ecx;
        char *ext = (char *)cpu.ecx + 8;
        *((struct FAT32DirectoryEntry*)cpu.ebx) = *(dir_table_linear_search(name, ext, cpu.edx));
    }  else if (cpu.eax == 9) {
        struct FAT32DriverRequest request = *(struct FAT32DriverRequest*)cpu.ebx;
        *((int8_t*)cpu.ecx) = custom_read_directory(request);
    }
}
