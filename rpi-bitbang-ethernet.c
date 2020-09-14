/* no header files :-D */

#define GPIO_BASE   0xFE200000
#define GPIO_SET0   7
#define GPIO_CLR0   10

static volatile unsigned int* GPIO = (volatile unsigned int*) GPIO_BASE;
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
    unsigned char version : 4;
    unsigned char ihl : 4;
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


struct frame {
    struct ethhdr ethhdr;
    struct iphdr iphdr;
    struct udphdr udphdr;
    
    unsigned char payload[];
} __attribute__((packed));

short ip_checksum(struct iphdr* iphdr);

extern void wait(int n);
extern void transmit_from_prefilled_gpio_set_or_clr(unsigned int* gpio_set_or_clrs, int bitcount);
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
    }
#ifndef __arm__
    for (int i = 0; i < (buflen * 8) * 2; i++) {
        printf("gpio_set_or_clrs[%d]: %x\n", i, gpio_set_or_clrs[i]);
    }
#else
    transmit_from_prefilled_gpio_set_or_clr(gpio_set_or_clrs, (buflen * 8) * 2);
#endif
}

void main(void) {
    const unsigned char source_ip[] = {192, 168, 1, 44};

    // destination set to my laptop's MAC and IP address; you should change this for yours.
    const unsigned char dest_ip[] = {192, 168, 1, 6};
    const unsigned int dest_mac[] = {0x78, 0x4F, 0x43, 0x88, 0x3B, 0xE2};

    char *payload = "Hello!\n";
    int payload_len = 7;

    unsigned char buf[1024];
    // Ethernet preamble
    buf[0] = 0x55; buf[1] = 0x55; buf[2] = 0x55; buf[3] = 0x55; buf[4] = 0x55; buf[5] = 0x55; buf[6] = 0x55;
    buf[7] = 0xD5; // start frame delimiter

    struct frame* frame = (struct frame*) &buf[8];
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
        iphdr->version = 4;
        iphdr->ihl = 5;
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
        udphdr->sport = 0x0010; // 1024;
        udphdr->dport = 0x0010; // 1024;
        udphdr->ulen = sizeof(frame->udphdr) + payload_len; udphdr->ulen = (udphdr->ulen>>8) | (udphdr->ulen<<8);
        udphdr->sum = 0; // don't care
    }
    unsigned char* buf_end = (unsigned char*) (frame + 1) + payload_len;

    gpio_set_as_output(GPIO_PIN_ETHERNET_TDp);
    gpio_set_as_output(GPIO_PIN_ETHERNET_TDm);

    gpio_set_as_output(42);
    gpio_set_as_output(26);

    gpio_set_value(GPIO_PIN_ETHERNET_TDm, 0);

    int v = 0;
    
    // 48 nops = 700 KHz
    // 24 nops = 1.2 MHz (0.4 us)
    // 12 nops = ~10 MHz (?)


    /* transmit(buf, buf_end); */
    /* gpio_set_value(42, 1); */
    /* for(;;){} */

    int nlps_sent = 0;
    for (;;) {
        /* gpio_set_value(GPIO_PIN_ETHERNET_TDp, v); v = !v; */

        /* wait(1000); // 1000 -> 1ms, 956Hz */
        /* wait(100); // 100 -> 0.1093ms, 9.188KHz */
        /* wait(1); // 1 -> 5us, 200KHz */

#ifdef __arm__
        normal_link_pulse();
#endif

        // sleep ~16ms
        /* wait(16000); */
        /* wait(100000); // 6s */
        /* wait(50000); // 3s */
        wait(75000); // ~16ms

        if (++nlps_sent % 125 == 0) {
            gpio_set_value(42, (v = !v));
            gpio_set_value(26, v);
            unsigned char bufsmall[] = {0x12, 0x34, 0x56};
            transmit(bufsmall, 3);
            /* transmit(buf, buf_end - buf); */
        }

        // see https://www.fpga4fun.com/10BASE-T3.html

        // FIXME: send buf contents
        /* for (unsigned char* addr = &buf[0]; addr != buf_end; addr++) { */
        /*     for (int i = 0; i < 8; i++) { */
        /*         int bit = (*addr >> i) & 1; */
        /*         if (bit) { */
        /*             gpio_set_value(GPIO_PIN_ETHERNET_TDp, 1); */
        /*             // CLOCK */
        /*             gpio_set_value(GPIO_PIN_ETHERNET_TDp, 0); */
        /*             // CLOCK */
        /*         } else { */
        /*             gpio_set_value(GPIO_PIN_ETHERNET_TDp, 0); */
        /*             // CLOCK */
        /*             gpio_set_value(GPIO_PIN_ETHERNET_TDp, 1); */
        /*             // CLOCK */
        /*         } */
        /*     } */
        /* } */
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
