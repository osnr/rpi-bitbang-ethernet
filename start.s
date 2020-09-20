.globl _start
_start:
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  
  mov sp, #0x8000000

  
  
  // FIXME: enable caches
  .equ    SCTLR_ENABLE_DATA_CACHE,         0x4
.equ    SCTLR_ENABLE_BRANCH_PREDICTION,  0x800
.equ    SCTLR_ENABLE_INSTRUCTION_CACHE,  0x1000

// Enable L1 Cache -------------------------------------------------------

// R0 = System Control Register
mrc p15,0,r0,c1,c0,0

// Enable caches and branch prediction
orr r0,#SCTLR_ENABLE_BRANCH_PREDICTION
orr r0,#SCTLR_ENABLE_DATA_CACHE
orr r0,#SCTLR_ENABLE_INSTRUCTION_CACHE

// System Control Register = R0
mcr p15,0,r0,c1,c0,0


  ldr r1, =__bss_start
  ldr r2, =__bss_end
  mov r3, #0
  _clear_bss_loop:
    cmp r1, r2
    strne r3, [r1], #4
    bne _clear_bss_loop

  bl main

hang: b hang
