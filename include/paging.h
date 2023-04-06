#ifndef _PAGING_H
#define _PAGING_H

#include "../lib/lib-header/stdtype.h"

#define PAGE_ENTRY_COUNT 1024
#define PAGE_FRAME_SIZE  (4*1024*1024)

// Operating system page directory, using page size PAGE_FRAME_SIZE (4 MiB)
extern struct PageDirectory _paging_kernel_page_directory;

/**
 * Page Directory Entry Flag, only first 8 bit
 *
 * @param present_bit       Indicate whether this entry is exist or not
 * @param write_bit         Whether the page is writable or read-only
 * @param user_supervisor   Whether the page is accessible by user or supervisor
 * @param write_through     Whether to enable write-through caching for this page
 * @param cache_disabled    Whether to disable caching for this page
 * @param accessed          Whether the page has been accessed
 * @param dirty             Whether the page has been written to
 * @param use_pagesize_4_mb Whether the page table entry is a page directory entry for a 4-MByte page
 */
struct PageDirectoryEntryFlag {
    uint8_t present_bit     : 1;
    uint8_t write_bit       : 1;
    uint8_t user_supervisor : 1;
    uint8_t write_through   : 1;
    uint8_t cache_disabled  : 1;
    uint8_t accessed        : 1;
    uint8_t dirty           : 1;
    uint8_t use_pagesize_4_mb : 1;
};

/**
 * Page Directory Entry, for page size 4 MB.
 * Check Intel Manual 3a - Ch 4 Paging - Figure 4-4 PDE: 4MB page
 *
 * @param flag            Contain 8-bit page directory entry flag
 * @param global_page     Is this page translation global (also cannot be flushed)
 * @param ignored         Ignored
 * @param pat             Whether the Page Attribute Table (PAT) feature is supported.
 * @param higher_address  Bits (M–1):32 of physical address of the 4-MByte page referenced by this entry
 * @param reserved        Reserved bits
 * @param lower_address   Bits 31:22 of physical address of the 4-MByte page referenced by this entry
 * Note:
 * - Assume "Bits 39:32 of address" (higher_address) is 8-bit and Reserved is 1
 * - "Bits 31:22 of address" is called lower_address in kit
 */
struct PageDirectoryEntry {
    struct PageDirectoryEntryFlag flag;
    uint16_t global_page    : 1;
    uint16_t ignored        : 3;
    uint16_t pat            : 1;
    uint16_t higher_address : 8;
    uint16_t reserved       : 1;
    uint16_t lower_address  : 10;
} __attribute__((packed));

/**
 * Page Directory, contain array of PageDirectoryEntry.
 * Note: This data structure not only can be manipulated by kernel,
 *   MMU operation, TLB hit & miss also affecting this data structure (dirty, accessed bit, etc).
 * Warning: Address must be aligned in 4 KB (listed on Intel Manual), use __attribute__((aligned(0x1000))),
 *   unaligned definition of PageDirectory will cause triple fault
 *
 * @param table Fixed-width array of PageDirectoryEntry with size PAGE_ENTRY_COUNT
 */
struct PageDirectory {
    struct PageDirectoryEntry table[PAGE_ENTRY_COUNT] __attribute__((aligned(0x1000)));
} __attribute__((packed));

/**
 * Containing page driver states
 * 
 * @param last_available_physical_addr Pointer to last empty physical addr (multiple of 4 MiB)
 */
struct PageDriverState {
    uint8_t *last_available_physical_addr;
} __attribute__((packed));

/**
 * update_page_directory_entry,
 * Edit _paging_kernel_page_directory with respective parameter
 * 
 * @param physical_addr Physical address to map
 * @param virtual_addr  Virtual address to map
 * @param flag          Page entry flags
 */
void update_page_directory_entry(void *physical_addr, void *virtual_addr, struct PageDirectoryEntryFlag flag);

/**
 * flush_single_tlb, 
 * invalidate page that contain virtual address in parameter
 * 
 * @param virtual_addr Virtual address to flush
 */
void flush_single_tlb(void *virtual_addr);

/**
 * Allocate user memory into specified virtual memory address.
 * Multiple call on same virtual address will unmap previous physical address and change it into new one.
 * 
 * @param  virtual_addr Virtual address to be mapped
 * @return int8_t       0 success, -1 for failed allocation
 */
int8_t allocate_single_user_page_frame(void *virtual_addr);

#endif