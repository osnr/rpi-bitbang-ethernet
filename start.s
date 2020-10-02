.globl _start
_start:
  // First, hang the other cores:
  // This may not be necessary (https://wiki.osdev.org/Raspberry_Pi_Bare_Bones#Pi_3.2C_4).
  // Return current CPU ID (0..3)
  mrc p15, 0, r0, c0, c0, 5       @ r0 = Multiprocessor Affinity Register (MPIDR)
  ands r0, #3              @ r0 = CPU ID (Bits 0..1)
  bne hang               @ If (CPU ID != 0) Branch To Infinite Loop (Core ID 1..3)

  cpsid if              @ Disable IRQ & FIQ

  // The Pi 4 boots into EL3 (secure monitor / sorta "HYP mode"),
  // which makes it annoying to enable the MMU (needed to set up
  // icache and dcache so things can be fast), so next, we will step
  // down into EL1 (supervisor / OS / "SVC mode").

  // This code assumes that we're already in HYP mode; it doesn't check.

  mrs  r0, cpsr
  bic  r0, r0, #0x1F          // clear mode bits
  // mask IRQ/FIQ bits (0x80 and 0x40) and set SVC mode (0x13)
  // (would it be sufficient to just set SVC mode, then `cpsid if`
  // after we `eret`? or would that be a race condition or sth?)
  orr  r0, r0, #0x13|0x80|0x40
  orr  r0, r0, #0x100          @ mask Abort bit
  // want to return to label 3: below when we leave HYP mode
  adr  lr, 3f
  msr  spsr_cxsf, r0
  msr ELR_hyp, lr
  eret
3:

  // You might ask (as I did)... why not just stay in EL3? You can
  // enable the MMU there fine -- you just need to use different
  // registers (HSCTLR instead of ACTLR). People say this would work,
  // but nobody really does it (hard to find examples). Apparently,
  // you'd then need extra boilerplate to handle exceptions
  // (https://www.raspberrypi.org/forums/viewtopic.php?t=268116). We
  // don't handle any exceptions right now, but not hard to imagine
  // wanting it later.

  // Now, we'll set up the runtime environment for C (stack pointer
  // and BSS segment):

  mov sp, #0x8000000

  ldr r1, =__bss_start
  ldr r2, =__bss_end
  mov r3, #0
  _clear_bss_loop:
    cmp r1, r2
    strne r3, [r1], #4
    bne _clear_bss_loop

  // Ready!

  bl main

hang: b hang
