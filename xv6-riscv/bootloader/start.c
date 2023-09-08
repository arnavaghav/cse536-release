/* These files have been taken from the open-source xv6 Operating System codebase (MIT License).  */

#include "types.h"
#include "param.h"
#include "layout.h"
#include "riscv.h"
#include "defs.h"
#include "buf.h"
#include "measurements.h"
#include <stdbool.h>

void main();
void timerinit();

/* entry.S needs one stack per CPU */
__attribute__ ((aligned (16))) char bl_stack[STSIZE * NCPU];

/* Context (SHA-256) for secure boot */
SHA256_CTX sha256_ctx;

/* Structure to collects system information */
struct sys_info {
  /* Bootloader binary addresses */
  uint64 bl_start;
  uint64 bl_end;
  /* Accessible DRAM addresses (excluding bootloader) */
  uint64 dr_start;
  uint64 dr_end;
  /* Kernel SHA-256 hashes */
  BYTE expected_kernel_measurement[32];
  BYTE observed_kernel_measurement[32];
};
struct sys_info* sys_info_ptr;

extern void _entry(void);

void panic(char *s)
{
  for(;;)
    ;
}

/* CSE 536: Boot into the RECOVERY kernel instead of NORMAL kernel
 * when hash verification fails. */
void setup_recovery_kernel(void) {
  uint64 recovery_kernel_load_addr       = find_kernel_load_addr(RECOVERYDISK);
  uint64 recovery_kernel_binary_size     = find_kernel_size(RECOVERYDISK);     
  uint64 recovery_kernel_entry           = find_kernel_entry_addr(RECOVERYDISK);

  struct buf recovery_kernel_buf;

  // Calculate the number of blocks to copy (excluding the first 4KB)
  uint64 num_blocks = (recovery_kernel_binary_size - 0x1000) / BSIZE;

  // Copy the recovery kernel binary excluding the first 4KB (which contains the ELF header)
  for (uint64 i = 0; i < num_blocks; i++) {
      recovery_kernel_buf.blockno = (0x1000 / BSIZE) + i;
      kernel_copy(RECOVERYDISK, &recovery_kernel_buf);
      memmove((void *)(recovery_kernel_load_addr + i * BSIZE), recovery_kernel_buf.data, BSIZE);
  }

  w_mepc((uint64) recovery_kernel_entry);

}

/* CSE 536: Function verifies if NORMAL kernel is expected or tampered. */
bool is_secure_boot(void) {
  bool verification = true;

  /* Read the binary and update the observed measurement 
   * (simplified template provided below) */
  
  /*
  
  // Initialize context
  sha256_init(&sha256_ctx);
  
  // Setup sturctures
  struct buf b;
  uint64 kernel_binary_size = find_kernel_size(NORMAL);
  uint64 num_blocks = kernel_binary_size / BSIZE;

  // Copy each block and update the hash
  for (uint64 i = 0; i < num_blocks; i++) {
    b.blockno = i;
    kernel_copy(NORMAL, &b);
    sha256_update(&sha256_ctx, (const unsigned char*) b.data, BSIZE);
  }

  // sha256_update(&sha256_ctx, (const unsigned char*) b.data, BSIZE);
    
  sha256_final(&sha256_ctx, sys_info_ptr->observed_kernel_measurement);

  //  1. Compare observed measurement with expected hash
  // verification = memcmp(sys_info_ptr->observed_kernel_measurement, sys_info_ptr->expected_kernel_measurement, 32) == 0;
  for (int i = 0; i < 32; i++) {
    if (sys_info_ptr->observed_kernel_measurement[i] != sys_info_ptr->expected_kernel_measurement[i]) {
      verification = false;
      break;
    }
  }

  //  2. Setup the recovery kernel if comparison fails
  if (!verification)
    setup_recovery_kernel();
  
  //  3. Copy expected kernel hash to the system information table 
  else {
    // memcpy(sys_info_ptr->expected_kernel_measurement, sys_info_ptr->observed_kernel_measurement, 32);
    for (int i = 0; i < 32; i++) {
      sys_info_ptr->expected_kernel_measurement[i] = sys_info_ptr->observed_kernel_measurement[i];
    }  
  }

  */

  return verification;
}

// entry.S jumps here in machine mode on stack0.
void start()
{
  /* CSE 536: Define the system information table's location. */
  // sys_info_ptr = (struct sys_info*) 0x80080000;
  sys_info_ptr = SYSINFOADDR;
  sys_info_ptr->bl_start = 0x80000000;
  sys_info_ptr->bl_end = (uint64)&end;
  sys_info_ptr->dr_start = 0x80000000;
  sys_info_ptr->dr_end = PHYSTOP;

  // keep each CPU's hartid in its tp register, for cpuid().
  int id = r_mhartid();
  w_tp(id);

  // set M Previous Privilege mode to Supervisor, for mret.
  unsigned long x = r_mstatus();
  x &= ~MSTATUS_MPP_MASK;
  x |= MSTATUS_MPP_S;
  w_mstatus(x);

  // disable paging
  w_satp(0);

  /* CSE 536: Unless kernelpmp[1-2] booted, allow all memory 
   * regions to be accessed in S-mode. */ 
  #if !defined(KERNELPMP1) && !defined(KERNELPMP2)
    w_pmpaddr0(0x3fffffffffffffull);
    w_pmpcfg0(0xf);
  #endif

  /* CSE 536: With kernelpmp1, isolate upper 10MBs using TOR */ 
  #if defined(KERNELPMP1)
    // Verion 1
    // w_pmpaddr0((PHYSTOP - (10 * 1024 * 1024)) >> 2);
    // w_pmpcfg0(0x18 | (1 << 7)); 
    w_pmpcfg0(0b00001111); // R, W, X bits set and A field set to TOR (b000)
    w_pmpaddr0((117 * 1024 * 1024) >> 2); // The address is right-shifted by 2 to fit into the register correctly
  #endif

  /* CSE 536: With kernelpmp2, isolate 118-120 MB and 122-126 MB using NAPOT */ 
  #if defined(KERNELPMP2)
    w_pmpaddr0(((118 * 1024 * 1024) >> 2) | ((1 << 3) - 1)); // Set NAPOT range
    w_pmpcfg0(0x18 | (1 << 7));
  #endif

  /* CSE 536: Verify if the kernel is untampered for secure boot */
  if (!is_secure_boot()) {
    /* Skip loading since we should have booted into a recovery kernel 
     * in the function is_secure_boot() */
    goto out;
  }
  
  /* CSE 536: Load the NORMAL kernel binary (assuming secure boot passed). */
  // uint64 kernload_start         = find_kernel_load_addr(NORMAL);
  
  // Version 1
  /* CSE 536: Write the correct kernel entry point */
  uint64 kernel_load_addr       = find_kernel_load_addr(NORMAL);
  uint64 kernel_binary_size     = find_kernel_size(NORMAL);     
  uint64 kernel_entry           = find_kernel_entry_addr(NORMAL);


  // Create a buffer to hold the data
  struct buf kernel_buf;

  // Calculate the number of blocks to copy (excluding the first 4KB)
  uint64 num_blocks = (kernel_binary_size - 0x1000) / BSIZE;
  
  // Copy the kernel binary excluding the first 4KB 
  for (uint64 i = 0; i < num_blocks; i++) {
    kernel_buf.blockno = (0x1000 / BSIZE) + i;
    kernel_copy(NORMAL, &kernel_buf);
    memmove((void *)(kernel_load_addr + i * BSIZE), kernel_buf.data, BSIZE);
  }

  w_mepc((uint64) kernel_entry);
 
 out:
  /* CSE 536: Provide system information to the kernel. */

  /* CSE 536: Send the observed hash value to the kernel (using sys_info_ptr) */

  // delegate all interrupts and exceptions to supervisor mode.
  w_medeleg(0xffff);
  w_mideleg(0xffff);
  w_sie(r_sie() | SIE_SEIE | SIE_STIE | SIE_SSIE);

  // return address fix
  uint64 addr = (uint64) panic;
  asm volatile("mv ra, %0" : : "r" (addr));

  // switch to supervisor mode and jump to main().
  asm volatile("mret");
}
