  buf .req r0
  buf_end .req r1

  byte .req r2
  i .req r3

  pin_ethernet_tdp .req r4
  pin_ethernet_tdm .req r5
  gpio_set .req r6
  gpio_clr .req r7

  .globl wait
wait:
  subs r0, #1
  bne wait
  bx lr

wait_halfbit_time:
  // this should take 50 nanoseconds (with MMU, caches, etc on)
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop

  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop

  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop

  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop

  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop

  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop

  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop

  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop

  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop

  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop

  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop

  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop

  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop

  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop

  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop

  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop

  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop

  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop

  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop

  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop
  bx lr

  .globl transmit
transmit:
  push {r4, r5, r6, r7, lr}

  mov pin_ethernet_tdp, #1
  lsl pin_ethernet_tdp, #20
  mov pin_ethernet_tdm, #1
  lsl pin_ethernet_tdm, #21
  ldr gpio_set, =0xFE20001C
  ldr gpio_clr, =0xFE200028

  byte_loop:
    ldrb byte, [buf], #1
    mov i, #1
    bit_loop:
      lsrs byte, i

      // bit = 1: LOW -> HIGH
      strcs pin_ethernet_tdm, [gpio_set]
      strcs pin_ethernet_tdp, [gpio_clr]
      blcs wait_halfbit_time
      strcs pin_ethernet_tdp, [gpio_set]
      strcs pin_ethernet_tdm, [gpio_clr]
      blcs wait_halfbit_time

      // bit = 0: HIGH -> LOW
      strcc pin_ethernet_tdp, [gpio_set]
      strcc pin_ethernet_tdm, [gpio_clr]
      blcc wait_halfbit_time
      strcc pin_ethernet_tdm, [gpio_set]
      strcc pin_ethernet_tdp, [gpio_clr]
      blcc wait_halfbit_time

      add i, #1
      cmp i, #8
      ble bit_loop

    cmp buf, buf_end
    blt byte_loop

  // Each packet needs to end with a "TP_IDL" (a positive pulse of
  // about 3 bit-times, followed by an idle period).
  str pin_ethernet_tdp, [gpio_set]
  str pin_ethernet_tdm, [gpio_clr]
  bl wait_halfbit_time
  bl wait_halfbit_time
  bl wait_halfbit_time
  bl wait_halfbit_time
  bl wait_halfbit_time
  bl wait_halfbit_time
  str pin_ethernet_tdp, [gpio_clr]
  str pin_ethernet_tdm, [gpio_clr]
  bl wait_halfbit_time
  bl wait_halfbit_time
  bl wait_halfbit_time
  bl wait_halfbit_time
  bl wait_halfbit_time
  bl wait_halfbit_time

  pop {r4, r5, r6, r7, lr}
  bx lr

  .globl normal_link_pulse
normal_link_pulse:
  push {r4, r5, r6, r7, lr}

  mov pin_ethernet_tdp, #1
  lsl pin_ethernet_tdp, #20
  mov pin_ethernet_tdm, #1
  lsl pin_ethernet_tdm, #21
  ldr gpio_set, =0xFE20001C
  ldr gpio_clr, =0xFE200028

  str pin_ethernet_tdp, [gpio_set]
  str pin_ethernet_tdm, [gpio_clr]
  bl wait_halfbit_time
  bl wait_halfbit_time
  str pin_ethernet_tdp, [gpio_clr]
  str pin_ethernet_tdm, [gpio_clr]

  pop {r4, r5, r6, r7, lr}
  bx lr
