  target_pin_ethernet_tdp .req r0
  target_pin_ethernet_tdm .req r1

  set_clr_pins_buf .req r2
  set_clr_pins_buf_end .req r3

  set_pins .req r4
  clr_pins .req r5

  gpio_set .req r6
  gpio_clr .req r7

  .globl wait
wait:
  // this is in assembly because I don't trust different compiler
  // options (and versions!!) to preserve its performance, and it's
  // important to keep its exec time the same-ish (at least on the Pi
  // 4B) so that normal link pulses stay spaced ~16ms apart.
  subs r0, #1
  bne wait
  bx lr

wait_halfbit_time:
  // the timing here is _really_ finicky. for example, right now, if I
  // add one NOP, it stops working on my Wi-Fi router (Netgear
  // R7000P), but it still works when plugged into a separate switch
  // or directly into an Ethernet adapter on my laptop. if I remove
  // one NOP, it still works on my router, but slightly less reliably?

  // you probably need to mess with these NOPs again (try
  // adding/removing 1-2) if you change something that changes
  // alignment or cache behavior of code or data, or if your room is a
  // different temperature than mine, or whatever.

  // I wonder if you could generate the right amount of NOPs
  // dynamically at startup by doing some timing. also wonder if this
  // would be better as a delay loop instead. (I started it as NOPs
  // because I was originally running with MMU and caches off, and a
  // delay loop was clearly way too expensive there.)

  // this should take ~50 nanoseconds (with MMU, caches, etc on)
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  nop; nop; nop; nop; nop; nop; nop; nop; nop; nop
  bx lr

  .globl transmit_from_set_clr_pins_buf
transmit_from_set_clr_pins_buf:
  push {r4, r5, r6, r7, lr}

  ldr gpio_set, =0xFE20001C
  ldr gpio_clr, =0xFE200028

  // This transmission might _need_ to be a uniform loop (like it is
  // now) to make the timing actually work. I couldn't get it to work
  // when I tried to do the Manchester encoding here, on the fly, as I
  // send the bits. It probably makes the timing too lopsided (in each
  // bit, one half-bit would take longer than the other half-bit,
  // because you're computing the Manchester stuff and going back to
  // the top of the loop while it's on), but that's just my guess.
  halfbit_loop:
    ldr set_pins, [set_clr_pins_buf], #4
    ldr clr_pins, [set_clr_pins_buf], #4

    str set_pins, [gpio_set]
    str clr_pins, [gpio_clr]

    bl wait_halfbit_time

    cmp set_clr_pins_buf, set_clr_pins_buf_end
    blt halfbit_loop

  // Each packet needs to end with a "TP_IDL" (a positive pulse of
  // about 3 bit-times, followed by an idle period).
  str target_pin_ethernet_tdp, [gpio_set] // go to positive (TD+ = 1, TD- = 0)
  str target_pin_ethernet_tdm, [gpio_clr]
  bl wait_halfbit_time
  bl wait_halfbit_time
  bl wait_halfbit_time
  bl wait_halfbit_time
  bl wait_halfbit_time
  bl wait_halfbit_time

  str target_pin_ethernet_tdp, [gpio_clr] // go to idle (TD+ = 0, TD- = 0)
  str target_pin_ethernet_tdm, [gpio_clr]
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

  ldr gpio_set, =0xFE20001C
  ldr gpio_clr, =0xFE200028

  str target_pin_ethernet_tdp, [gpio_set] // go to positive (TD+ = 1, TD- = 0)
  str target_pin_ethernet_tdm, [gpio_clr]
  bl wait_halfbit_time
  bl wait_halfbit_time
  str target_pin_ethernet_tdp, [gpio_clr] // go to idle (TD+ = 0, TD- = 0)
  str target_pin_ethernet_tdm, [gpio_clr]

  pop {r4, r5, r6, r7, lr}
  bx lr
