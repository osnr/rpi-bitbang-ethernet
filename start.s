.globl _start
_start:
  mov sp, #0x8000000

  ldr r1, =__bss_start
  ldr r2, =__bss_end
  mov r3, #0
  _clear_bss_loop:
    str r3, [r1]
    add r1, #4
    cmp r1, r2
    bne _clear_bss_loop

  bl main

hang: b hang
