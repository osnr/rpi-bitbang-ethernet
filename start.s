
@ Standard definitions of Mode bits and Interrupt (I & F) flags in PSRs
.set  MODE_USR, 0x10				@ User Mode
.set  MODE_FIQ, 0x11				@ FIQ Mode
.set  MODE_IRQ, 0x12				@ IRQ Mode
.set  MODE_SVC, 0x13				@ Supervisor Mode
.set  MODE_ABT, 0x17				@ Abort Mode
.set  MODE_UND, 0x1B				@ Undefined Mode
.set  MODE_SYS, 0x1F				@ System Mode

.set  I_BIT, 0x80					@ when I bit is set, IRQ is disabled
.set  F_BIT, 0x40					@ when F bit is set, FIQ is disabled

.globl _start
_start:
  	@ Return current CPU ID (0..3)
	mrc p15, 0, r0, c0, c0, 5 			@ r0 = Multiprocessor Affinity Register (MPIDR)
	ands r0, #3							@ r0 = CPU ID (Bits 0..1)
	bne hang 							@ If (CPU ID != 0) Branch To Infinite Loop (Core ID 1..3)
	cpsid if							@ Disable IRQ & FIQ
    @ Check for HYP mode
	mrs	r0 , cpsr
	eor	r0, r0, #0x1A
	tst	r0, #0x1F
	bic	r0 , r0 , #0x1F					@ clear mode bits
	orr	r0 , r0 , #MODE_SVC|I_BIT|F_BIT	@ mask IRQ/FIQ bits and set SVC mode
	bne	2f								@ branch if not HYP mode
	orr	r0, r0, #0x100					@ mask Abort bit
	adr	lr, 3f
	msr	spsr_cxsf, r0
	.word	0xE12EF30E					@ msr ELR_hyp, lr
	.word	0xE160006E					@ eret
2:	msr cpsr_c, r0
3:

  mov sp, #0x8000000

  ldr r1, =__bss_start
  ldr r2, =__bss_end
  mov r3, #0
  _clear_bss_loop:
    cmp r1, r2
    strne r3, [r1], #4
    bne _clear_bss_loop

  bl main

hang: b hang
