  addr .req r0
  buf_end .req r1

  word .req r2
  i .req r3

  gpio_stamp .req r4
  gpio_set_or_clr .req r5
  gpio_set .req r6
  gpio_clr .req r7

  .globl wait
wait:
  subs r0, #1
  bne wait
  bx lr

wait_bit_time: // 6 nops is baseline
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
  bx lr

  .globl transmit_from_prefilled_gpio_set_or_clr
transmit_from_prefilled_gpio_set_or_clr:
  push {r4, r5, r6, r7, lr}
  
  mov gpio_stamp, #1
  lsl gpio_stamp, #20
  ldr gpio_set, =0xFE20001C
  ldr gpio_clr, =0xFE200028

  bit_loop:
    ldr gpio_set_or_clr, [addr], #4
    str gpio_stamp, [gpio_set_or_clr]
    bl wait_bit_time

    cmp addr, buf_end
    blt bit_loop

  // Each packet needs to end with a "TP_IDL" (a positive pulse of
  // about 3 bit-times, followed by an idle period).
  str gpio_stamp, [gpio_set]
  bl wait_bit_time
  bl wait_bit_time
  bl wait_bit_time
  str gpio_stamp, [gpio_clr]
  bl wait_bit_time
  bl wait_bit_time
  bl wait_bit_time

  pop {r4, r5, r6, r7, lr}
  bx lr

  .globl normal_link_pulse
normal_link_pulse:
  push {r4, r5, r6, r7, lr}

  mov gpio_stamp, #1
  lsl gpio_stamp, #20
  ldr gpio_set, =0xFE20001C
  ldr gpio_clr, =0xFE200028

  str gpio_stamp, [gpio_set]
  bl wait_bit_time
  str gpio_stamp, [gpio_clr]

  pop {r4, r5, r6, r7, lr}
  bx lr
