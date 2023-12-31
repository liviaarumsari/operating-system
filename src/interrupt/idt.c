#include "../../include/idt.h"

// Declare empty IDT
struct IDT interrupt_descriptor_table;

// Declare empty IDTR
struct IDTR _idt_idtr = {
    .size = sizeof(interrupt_descriptor_table) - 1,
    .address = &interrupt_descriptor_table
};

void initialize_idt(void) {
    // Iterate all isr_stub_table and setting all IDT entry
    for (int i = 0; i < ISR_STUB_TABLE_LIMIT; i++) {
        set_interrupt_gate(i, isr_stub_table[i], GDT_KERNEL_CODE_SEGMENT_SELECTOR, 0);
    }

    for (int i = 0x30; i <= 0x3F; i++) {
        set_interrupt_gate(i, isr_stub_table[i], GDT_KERNEL_CODE_SEGMENT_SELECTOR, 0x3);
    }
    
    // Load IDTR with lidt instruction
    __asm__ volatile("lidt %0" : : "m"(_idt_idtr));
    
    // Enable interrupts
    __asm__ volatile("sti");
}

void set_interrupt_gate(uint8_t int_vector, void *handler_address, uint16_t gdt_seg_selector, uint8_t privilege) {
    struct IDTGate *idt_int_gate = &interrupt_descriptor_table.table[int_vector];

    // Set handler offset
    idt_int_gate->offset_low = ((uint32_t)handler_address) & 0xFFFF;
    idt_int_gate->offset_high = ((uint32_t)handler_address >> 16) & 0xFFFF;

    // Target system 32-bit and flag this as valid interrupt gate
    idt_int_gate->segment       = gdt_seg_selector;
    idt_int_gate->_reserved     = 0;
    idt_int_gate->_r_bit_1      = INTERRUPT_GATE_R_BIT_1;
    idt_int_gate->_r_bit_2      = INTERRUPT_GATE_R_BIT_2;
    idt_int_gate->gate_32       = 1;
    idt_int_gate->_r_bit_3      = INTERRUPT_GATE_R_BIT_3;
    idt_int_gate->privilege     = privilege;
    idt_int_gate->valid_bit     = 1;
}