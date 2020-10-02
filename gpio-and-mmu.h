#ifndef UTIL_H
#define UTIL_H

#define GPIO_BASE   0xFE200000
#define GPIO_SET0   7
#define GPIO_CLR0   10

// this is a lot of `volatile`s
static volatile unsigned int* volatile GPIO = (volatile unsigned int*) GPIO_BASE;
// These functions are used for setup and for debug stuff in C, but
// are not used by the time-sensitive assembly-language transmission
// routines in transmit.s (which just poke at the GPIO registers
// directly)
void gpio_set_as_output(int pin) {
    GPIO[pin/10] &= ~(7 << (3 * (pin % 10)));
    GPIO[pin/10] |= 1 << (3 * (pin % 10));
}
void gpio_set_value(int pin, int value) {
    if (value) { GPIO[GPIO_SET0 + pin/32] = 1 << (pin % 32); }
    else { GPIO[GPIO_CLR0 + pin/32] = 1 << (pin % 32); }
}

void enable_mmu(void) {
    // from https://www.raspberrypi.org/forums/viewtopic.php?t=65922
    // also see: http://maisonikkoku.com/jaystation2/chapter_05.html

    static volatile __attribute__ ((aligned (0x4000))) unsigned PageTable[4096];

    unsigned base;
    for (base = 0; base < 512; base++)
        {
            // http://www.cs.otago.ac.nz/cosc440/readings/Cortex-A.pdf#page=137
            // you know it's a section translation entry (mapping a 1MB region)
            // instead of a pointer to 2nd level page table because the LSB is 0.
            // outer and inner write back, write allocate, shareable
            PageTable[base] = base << 20 | 0x1140E;
        }
    for (; base < 4096; base++)
        {
            // shared device, never execute
            PageTable[base] = base << 20 | 0x10416;
        }

    // restrict cache size to 16K (no page coloring)
    unsigned auxctrl;
    __asm__ volatile ("mrc p15, 0, %0, c1, c0,  1" : "=r" (auxctrl));
    auxctrl |= 1 << 6;
    __asm__ volatile ("mcr p15, 0, %0, c1, c0,  1" :: "r" (auxctrl));

    // set domain 0 to client
    __asm__ volatile ("mcr p15, 0, %0, c3, c0, 0" :: "r" (1));

    // always use TTBR0
    __asm__ volatile ("mcr p15, 0, %0, c2, c0, 2" :: "r" (0));

    // set TTBR0 (page table walk inner cacheable, outer non-cacheable, shareable memory)
    __asm__ volatile ("mcr p15, 0, %0, c2, c0, 0" :: "r" (3 | (unsigned) &PageTable));

    // invalidate data cache and flush prefetch buffer
    // (this crashes the Pi 4. I don't think we need it, though?
    // http://classweb.ece.umd.edu/enee447.S2019/baremetal_boot_code_for_ARMv8_A_processors.pdf#page=21
    // "In ARMv8-A processors and most ARMv7-A processors, you do not
    // have to do this because hardware automatically invalidates all
    // cache RAMs after reset")
    // various places online implement an alternative method for Pi 2/3:
    // https://github.com/xlar54/PiBASIC/blob/beab4f1b108142e33e6baaeac5b34ea038da6407/src/cache.c#L270
    // (who came up with that code originally?)
    /* __asm__ volatile ("mcr p15, 0, %0, c7, c5,  4" :: "r" (0) : "memory"); */
    /* __asm__ volatile ("mcr p15, 0, %0, c7, c6,  0" :: "r" (0) : "memory"); */

    // enable MMU, L1 cache and instruction cache, L2 cache, write buffer,
    //   branch prediction and extended page table on
    unsigned mode;
    __asm__ volatile ("mrc p15,0,%0,c1,c0,0" : "=r" (mode));
    mode |= 0x0480180D;
    __asm__ volatile ("mcr p15,0,%0,c1,c0,0" :: "r" (mode) : "memory");
}

#endif
