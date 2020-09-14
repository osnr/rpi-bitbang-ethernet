  addr .req r0
  buf_end .req r1

  word .req r2
  i .req r3

  gpio_stamp .req r4
  gpio_set .req r5
  gpio_clr .req r6

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

  .globl transmit
transmit:
  push {r4, r5, r6, lr}
  
  mov gpio_stamp, #1
  lsl gpio_stamp, #20
  ldr gpio_set, =0xFE20001C
  ldr gpio_clr, =0xFE200028

  word_loop:
    ldr word, [addr]
    rev word, word

    mov i, #0
    bit_loop:
      lsrs word, i
      strcs gpio_stamp, [gpio_set]
      strcc gpio_stamp, [gpio_clr]
      bl wait_bit_time

      strcs gpio_stamp, [gpio_clr]
      strcc gpio_stamp, [gpio_set]
      bl wait_bit_time

      add i, #1
      cmp i, #32
      blt bit_loop

    add addr, #4
    cmp addr, buf_end
    blt word_loop

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

  pop {r4, r5, r6, lr}
  bx lr

  .globl normal_link_pulse
normal_link_pulse:
  push {r4, r5, r6, lr}

  mov gpio_stamp, #1
  lsl gpio_stamp, #20
  ldr gpio_set, =0xFE20001C
  ldr gpio_clr, =0xFE200028

  str gpio_stamp, [gpio_set]
  bl wait_bit_time
  str gpio_stamp, [gpio_clr]

  pop {r4, r5, r6, lr}
  bx lr
