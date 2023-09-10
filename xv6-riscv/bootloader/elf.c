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


uint64 find_kernel_load_addr(enum kernel ktype) {
    /* CSE 536: Get kernel load address from headers */
    
    uint64 base_addr = (ktype == NORMAL) ? RAMDISK : RECOVERYDISK;
    kernel_elfhdr = (struct elfhdr*) base_addr;

    // Step 1: Check that the ELF header is valid
    if (kernel_elfhdr->magic != ELF_MAGIC) {
        panic("Invalid ELF file");
    }
    
    // Step 2: Grab the offset and size of the program headers from the ELF header
    uint64 phoff = kernel_elfhdr->phoff;
    uint64 phsize = kernel_elfhdr->phentsize;

    // Step 3: Navigate to the .text section's header in the program headers
    kernel_phdr = (struct proghdr*) (base_addr + phoff + phsize);

    // Step 4: Retrieve the starting address of the .text section from the program header
    uint64 kernload_start = kernel_phdr->vaddr;

    return kernload_start;
}

uint64 find_kernel_entry_addr(enum kernel ktype) {
    /* CSE 536: Get kernel entry point from headers */
    
    uint64 base_addr = (ktype == NORMAL) ? RAMDISK : RECOVERYDISK;
    struct elfhdr* kernel_elfhdr = (struct elfhdr*) base_addr;
    
    uint64 entry_addr = kernel_elfhdr->entry;
    return entry_addr;
}

uint64 find_kernel_size(enum kernel ktype) {
    /* CSE 536: Get kernel size from headers */
    
    uint64 base_addr = (ktype == NORMAL) ? RAMDISK : RECOVERYDISK;
    struct elfhdr* kernel_elfhdr = (struct elfhdr*) base_addr;
    
    int total_size = kernel_elfhdr->shoff + (kernel_elfhdr->shnum * kernel_elfhdr->shentsize);
    return total_size;
}