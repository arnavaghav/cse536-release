#include "types.h"
#include "param.h"
#include "layout.h"
#include "riscv.h"
#include "defs.h"
#include "buf.h"
#include "elf.h"

#include <stdbool.h>

struct elfhdr* kernel_elfhdr;
struct proghdr* kernel_phdr;


// Version 1
uint64 find_kernel_load_addr(enum kernel ktype) {
    /* CSE 536: Get kernel load address from headers */
    kernel_elfhdr = (struct elfhdr*) RAMDISK;

    // Step 1: Check that the ELF header is valid
    if (kernel_elfhdr->magic != ELF_MAGIC) {
        panic("Invalid ELF file");
    }
    // Step 2: Grab the offset and size of the program headers from the ELF header
    uint64 phoff = kernel_elfhdr->phoff;
    uint64 phsize = kernel_elfhdr->phentsize;

    // Step 3: Navigate to the .text section's header in the program headers
    kernel_phdr = (struct proghdr*) (RAMDISK + phoff + phsize);

    // Step 4: Retrieve the starting address of the .text section from the program header
    uint64 kernload_start = kernel_phdr->vaddr;

    return kernload_start;
}

uint64 find_kernel_size(enum kernel ktype) {
    /* CSE 536: Get kernel binary size from headers */
    
    // Version 2
    struct elfhdr* kernel_elfhdr = (struct elfhdr*) RAMDISK;

    // Step 2: Find the offset to the section header table and the number of entries
    uint64 shoff = kernel_elfhdr->shoff;
    uint16 shnum = kernel_elfhdr->shnum;
    uint16 shentsize = kernel_elfhdr->shentsize;

    // Step 3: Calculate the size of the binary as the offset of the last section header plus its size
    uint64 kernel_size = shoff + (shnum * shentsize);

    return kernel_size;

}

uint64 find_kernel_entry_addr(enum kernel ktype) {
    /* CSE 536: Get kernel entry point from headers */
    struct elfhdr* kernel_elfhdr = (struct elfhdr*) RAMDISK;
    uint64 entry_addr = kernel_elfhdr->entry;
    return entry_addr;
}

// uint64 find_kernel_load_addr(enum kernel type) {
//     kernel_elfhdr = (struct elfhdr*) RAMDISK;
//     uint64 elfhdr_phoff = kernel_elfhdr->phoff;	
//     uint64 elfhdr_psize = kernel_elfhdr->phentsize;
//     kernel_phdr = (struct proghdr*) (RAMDISK + elfhdr_phoff + elfhdr_psize);
//     return kernel_phdr->vaddr;
// }

// uint64 find_kernel_size(enum kernel type) {
//     kernel_elfhdr = (struct elfhdr*)RAMDISK;
//     int total_size = kernel_elfhdr->shoff + (kernel_elfhdr->shnum * kernel_elfhdr->shentsize);
//     return total_size;
// }

// uint64 find_kernel_entry_addr(enum kernel type) {
//     kernel_elfhdr = (struct elfhdr*)RAMDISK;
//     return kernel_elfhdr->entry;
// }

