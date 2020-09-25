#ifndef UTIL_H
#define UTIL_H

#define GPIO_BASE   0xFE200000
#define GPIO_SET0   7
#define GPIO_CLR0   10

static volatile unsigned int* volatile GPIO = (volatile unsigned int*) GPIO_BASE;
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
    static volatile __attribute__ ((aligned (0x4000))) unsigned PageTable[4096];

    unsigned base;
    for (base = 0; base < 512; base++)
        {
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
