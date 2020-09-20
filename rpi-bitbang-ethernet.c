/* no header files :-D */

#define GPIO_BASE   0xFE200000
#define GPIO_SET0   7
#define GPIO_CLR0   10

static volatile unsigned int* volatile GPIO = (volatile unsigned int*) GPIO_BASE;
void gpio_set_as_output(int pin) {
#ifdef __arm__
    GPIO[pin/10] &= ~(7 << (3 * (pin % 10)));
    GPIO[pin/10] |= 1 << (3 * (pin % 10));
#endif
}
void gpio_set_value(int pin, int value) {
#ifdef __arm__
    if (value) { GPIO[GPIO_SET0 + pin/32] = 1 << (pin % 32); }
    else { GPIO[GPIO_CLR0 + pin/32] = 1 << (pin % 32); }
#endif
}

#define GPIO_PIN_ETHERNET_TDp  20
#define GPIO_PIN_ETHERNET_TDm  21

struct ethhdr {
    unsigned char dmac[6];
    unsigned char smac[6];
    unsigned short ethertype;
} __attribute__((packed));

struct iphdr {
    unsigned char ihl_and_version;
    unsigned char tos;
    unsigned short len;
    unsigned short id;
    unsigned short flags : 3;
    unsigned short frag_offset : 13;
    unsigned char ttl;
    unsigned char proto;
    unsigned short csum;
    unsigned char saddr[4];
    unsigned char daddr[4];
} __attribute__((packed));

struct udphdr {
    unsigned short sport;
    unsigned short dport;
    short ulen;
    unsigned short sum;
} __attribute__((packed));


struct framehdr {
    struct ethhdr ethhdr;
    struct iphdr iphdr;
    struct udphdr udphdr;
    
    unsigned char payload[];
} __attribute__((packed));

struct frametlr {
    unsigned int fcs;
} __attribute__((packed));

short ip_checksum(struct iphdr* iphdr);

extern void wait(int n);
extern void transmit_from_prefilled_gpio_set_or_clr(unsigned int* gpio_set_or_clrs, int halfbitcount);
extern void normal_link_pulse(void);

void transmit(unsigned char* buf, int buflen) {
    unsigned int gpio_set_or_clrs[(buflen * 8) * 2];
    for (int i = 0; i < buflen; i++) {
        for (int j = 0; j < 8; j++) {
            int bit = (buf[i] >> j) & 1;
            if (bit) { // low then high
                gpio_set_or_clrs[(i * 8 + j) * 2] = (unsigned int) &GPIO[GPIO_CLR0];
                gpio_set_or_clrs[(i * 8 + j) * 2 + 1] = (unsigned int) &GPIO[GPIO_SET0];
            } else { // high then low
                gpio_set_or_clrs[(i * 8 + j) * 2] = (unsigned int) &GPIO[GPIO_SET0];
                gpio_set_or_clrs[(i * 8 + j) * 2 + 1] = (unsigned int) &GPIO[GPIO_CLR0];
            }
        }
#ifndef __arm__
        printf("%02x ", buf[i]);
#endif
    }
#ifndef __arm__
    printf("\n");
#endif
#ifndef __arm__
    for (int i = 0; i < (buflen * 8) * 2; i++) {
        /* printf("gpio_set_or_clrs[%02d]: %08x (%s)\n", i, gpio_set_or_clrs[i], gpio_set_or_clrs[i] == &GPIO[GPIO_SET0] ? "high" : "low"); */
    }
#else
    gpio_set_value(19, 1);
    transmit_from_prefilled_gpio_set_or_clr(gpio_set_or_clrs, (buflen * 8) * 2);
    gpio_set_value(19, 0);
#endif
}


/* PL011 UART registers */
#define MMIO_BASE   0xFE000000
#define UART0_DR        ((volatile unsigned int*)(MMIO_BASE+0x00201000))
#define UART0_FR        ((volatile unsigned int*)(MMIO_BASE+0x00201018))
#define UART0_IBRD      ((volatile unsigned int*)(MMIO_BASE+0x00201024))
#define UART0_FBRD      ((volatile unsigned int*)(MMIO_BASE+0x00201028))
#define UART0_LCRH      ((volatile unsigned int*)(MMIO_BASE+0x0020102C))
#define UART0_CR        ((volatile unsigned int*)(MMIO_BASE+0x00201030))
#define UART0_IMSC      ((volatile unsigned int*)(MMIO_BASE+0x00201038))
#define UART0_ICR       ((volatile unsigned int*)(MMIO_BASE+0x00201044))
void uart_putc(unsigned int c) {
    /* wait until we can send */
    do{__asm__ volatile("nop");}while(*UART0_FR&0x20);
    /* write the character to the buffer */
    *UART0_DR=c;
}
void uart_puts(char* s) {
    for (int i = 0; i < 2; i++) uart_putc(s[i]);
}

void EnableMMU (void)
{
#ifdef __arm__
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
#endif
}

unsigned int crc32b(unsigned char *message, int messagelen) {
   int i, j;
   unsigned int byte, crc, mask;

   i = 0;
   crc = 0xFFFFFFFF;
   while (i < messagelen) {
      byte = message[i];            // Get next byte.
      crc = crc ^ byte;
      for (j = 7; j >= 0; j--) {    // Do eight times.
         mask = -(crc & 1);
         crc = (crc >> 1) ^ (0xEDB88320 & mask);
      }
      i = i + 1;
   }
   return ~crc;
}

void main(void) {
    EnableMMU();

    const unsigned char source_ip[] = {192, 168, 1, 44};

    // destination set to my laptop's MAC and IP address; you should change this for yours.
    const unsigned char dest_ip[] = {192, 168, 1, 6};
    const unsigned int dest_mac[] = {0x78, 0x4F, 0x43, 0x88, 0x3B, 0xE2};

    char *payload = "Hello! This payload needs to be fairly long to work, so I'm gonna stretch it out a bit\n";
    int payload_len = 87;

    unsigned char buf[1024];
    // Ethernet preamble
    buf[0] = 0x55; buf[1] = 0x55; buf[2] = 0x55; buf[3] = 0x55; buf[4] = 0x55; buf[5] = 0x55; buf[6] = 0x55;
    buf[7] = 0xD5; // start frame delimiter

    struct framehdr* frame = (struct framehdr*) &buf[8];
    for (int i = 0; i < payload_len; i++) { frame->payload[i] = payload[i]; }

    {
        struct ethhdr* ethhdr = &frame->ethhdr;
        // (the proper way to do this would be to do an ARP thing to
        // online resolve MAC addr from IP addr ?)
        for (int i = 0; i < 6; i++) ethhdr->dmac[i] = dest_mac[i];

        unsigned char* s = ethhdr->smac;
        // FIXME: ? made-up 'source MAC address' for the Pi.
        s[0] = 0x00; s[1] = 0x12; s[2] = 0x34; s[3] = 0x56; s[4] = 0x78; s[5] = 0x90;

        // ETH_P_IP / 'this Ethernet frame contains an IP datagram'
        ethhdr->ethertype = 0x0008; // 0x0800; 
    }
    { // see RFC791: https://tools.ietf.org/html/rfc791
      // also see concretely: https://www.fpga4fun.com/10BASE-T2.html
        struct iphdr* iphdr = &frame->iphdr;
        iphdr->ihl_and_version = 0x45; // version 4, ihl 5
        iphdr->tos = 0x00; // don't care
        iphdr->len = sizeof(frame->iphdr) + sizeof(frame->udphdr) + payload_len; iphdr->len = (iphdr->len>>8) | (iphdr->len<<8);
        iphdr->id = 0x00; // don't care
        iphdr->flags = 0x00; // don't care
        iphdr->frag_offset = 0; // this is the only datagram in the fragment (?)
        iphdr->ttl = 8;
        iphdr->proto = 0x11; // UDP
        iphdr->csum = 0; // will fixup later
        for (int i = 0; i < 4; i++) iphdr->saddr[i] = source_ip[i];
        for (int i = 0; i < 4; i++) iphdr->daddr[i] = dest_ip[i];

        iphdr->csum = ip_checksum(iphdr);
        iphdr->csum = (iphdr->csum>>8) | (iphdr->csum<<8);
    }
    { // see RFC768: https://tools.ietf.org/html/rfc768
        struct udphdr* udphdr = &frame->udphdr;
        udphdr->sport = 0x0004; // 1024;
        udphdr->dport = 0x0004; // 1024;
        udphdr->ulen = sizeof(frame->udphdr) + payload_len; udphdr->ulen = (udphdr->ulen>>8) | (udphdr->ulen<<8);
        udphdr->sum = 0; // don't care
    }
    unsigned char* buf_end = (unsigned char*) (frame + 1) + payload_len;
    struct frametlr* frametlr = (struct frametlr*) buf_end;

    frametlr->fcs = crc32b((unsigned char*) frame, buf_end - (unsigned char*) frame);
    
    buf_end += sizeof(*frametlr);

    gpio_set_as_output(GPIO_PIN_ETHERNET_TDp);
    gpio_set_as_output(GPIO_PIN_ETHERNET_TDm);
    gpio_set_as_output(16);

    gpio_set_as_output(42);
    gpio_set_as_output(19);

    gpio_set_value(GPIO_PIN_ETHERNET_TDm, 0);

    int v = 0;

    int nlps_sent = 0;
    for (;;) {
#ifdef __arm__
        normal_link_pulse();
#endif

        wait(75000 * 70); // ~16ms

        if (++nlps_sent % 125 == 0) {
            gpio_set_value(42, (v = !v));
            transmit(buf, buf_end - buf);
        }

        // see https://www.fpga4fun.com/10BASE-T3.html
    }
    
}

short ip_checksum(struct iphdr* iphdr) {
    unsigned short* addr = (unsigned short*) iphdr;
    int count = sizeof(*iphdr);
    // from https://tools.ietf.org/html/rfc1071
    /* Compute Internet Checksum for "count" bytes
     *         beginning at location "addr".
     */
    register long sum = 0;

    while( count > 1 )  {
        /*  This is the inner loop */
        sum += * addr++;
        count -= 2;
    }

    /*  Add left-over byte, if any */
    if( count > 0 )
        sum += * (unsigned char *) addr;

    /*  Fold 32-bit sum to 16 bits */
    while (sum>>16)
        sum = (sum & 0xffff) + (sum >> 16);

    return ~sum;
}
