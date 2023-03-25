#include "../../lib/lib-header/stdtype.h"
#include "../../include/gdt.h"

/**
 * global_descriptor_table, predefined GDT.
 * Initial SegmentDescriptor already set properly according to GDT definition in Intel Manual & OSDev.
 * Table entry : [{Null Descriptor}, {Kernel Code}, {Kernel Data (variable, etc)}, ...].
 */
struct GlobalDescriptorTable global_descriptor_table = {
    .table = {
        {
            // Null descriptor
            .segment_low = 0,
            .base_low = 0,
            .base_mid = 0,
            .type_bit = 0,
            .non_system = 0,
            .dpl = 0,
            .present = 0,
            .segment_limit_high = 0,
            .avl = 0,
            .l = 0,
            .db = 0,
            .granularity = 0,
            .base_high = 0
        },
        {
            // Kernel code segment
            .segment_low = 0xFFFF,
            .base_low = 0,
            .base_mid = 0,
            .type_bit = 0xA,            // Code, not accessed, readable, not conforming
            .non_system = 1,            // Code segment
            .dpl = 0,                   // Kernel privilege level
            .present = 1,               // Segment is valid
            .segment_limit_high = 0xF,
            .avl = 0,
            .l = 1,                     // 32 bit operand size
            .db = 1,                    // 32 bit code segment
            .granularity = 1,           // 4KB granularity
            .base_high = 0
        },
        {
            // Kernel data segment
            .segment_low = 0xFFFF,
            .base_low = 0,
            .base_mid = 0,
            .type_bit = 0x2,            // Data, not accessed, writable, direction up
            .non_system = 1,            // Data segment
            .dpl = 0,                   // Kernel privilege level
            .present = 1,               // Segment is valid
            .segment_limit_high = 0xF,
            .avl = 0,
            .l = 0,
            .db = 1,
            .granularity = 1,
            .base_high = 0
        }
    }
};

/**
 * _gdt_gdtr, predefined system GDTR. 
 * GDT pointed by this variable is already set to point global_descriptor_table above.
 * From: https://wiki.osdev.org/Global_Descriptor_Table, GDTR.size is GDT size minus 1.
 */
struct GDTR _gdt_gdtr = {
    .size = sizeof(struct GlobalDescriptorTable) - 1,
    .address = &global_descriptor_table
};
