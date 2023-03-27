#include "../lib/lib-header/portio.h"
#include "../lib/lib-header/stdtype.h"
#include "../lib/lib-header/stdmem.h"
#include "../include/gdt.h"
#include "../include/framebuffer.h"
#include "../include/kernel_loader.h"
#include "../include/idt.h"
#include "../include/interrupt.h"
#include "../include/keyboard.h"

void kernel_setup(void) {
    enter_protected_mode(&_gdt_gdtr);
    pic_remap();
    initialize_idt();
    framebuffer_clear();
    framebuffer_set_cursor(0, 0);
    __asm__("int $0x4");
    while (TRUE) {
        keyboard_state_activate();
    }
}