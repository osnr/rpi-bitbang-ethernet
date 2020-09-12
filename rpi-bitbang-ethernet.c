/* no header files :-D */

#define GPIO_BASE     0xFE200000
#define GPIO_SET0   7
#define GPIO_CLR0   10

static volatile unsigned int* GPIO = (unsigned int*) GPIO_BASE;
void gpio_set_as_output(int pin) {
    GPIO[pin/10] &= ~(7 << (3 * (pin % 10)));
    GPIO[pin/10] |= 1 << (3 * (pin % 10));
}
void gpio_set_value(int pin, int value) {
    if (value) { GPIO[GPIO_SET0 + pin/32] = 1 << (pin % 32); }
    else { GPIO[GPIO_CLR0 + pin/32] = 1 << (pin % 32); }
}
void wait(int n) {
    // 0x3F0000 = about 3 seconds?
    for (volatile int i = 0; i < n; i++) {}
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
    unsigned int saddr;
    unsigned int daddr;
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

void main(void) {
    const unsigned char source_ip[] = {192, 168, 1, 44};

    // destination set to my laptop's MAC and IP address; you should change this for yours.
    const unsigned char dest_ip[] = {192, 168, 1, 6};
    const unsigned int dest_mac[] = {0x78, 0x4F, 0x43, 0x88, 0x3B, 0xE2};

    /* __asm__( */
    /*         "ldr r0,=0xFE200000   \n" */
    /*         "mov r1, #1           \n" */
    /*         "lsl r1,#6            \n"  /\* -> 001 000 000 *\/ */
    /*         "str r1,[r0,#0x10]    \n" */
    /*         "mov r1,#1            \n" */
    /*         "lsl r1,#10           \n" */

    /*         "str r1,[r0,#0x2C]  \n" */
    /*         : : : "r0","r1" ); */

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
        ethhdr->ethertype = 0x0800; 
    }
    { // see RFC791: https://tools.ietf.org/html/rfc791
      // also see concretely: https://www.fpga4fun.com/10BASE-T2.html
        struct iphdr* iphdr = &frame->iphdr;
        iphdr->version = 4;
        iphdr->ihl = 5;
        iphdr->tos = 0x00; // don't care
        iphdr->len = sizeof(frame->iphdr) + sizeof(frame->udphdr) + payload_len;
        iphdr->id = 0x00; // don't care
        iphdr->flags = 0x00; // don't care
        iphdr->frag_offset = 0; // this is the only datagram in the fragment (?)
        iphdr->ttl = 8;
        iphdr->proto = 0x11; // UDP
        iphdr->csum = 0; // will fixup later
        iphdr->saddr = (source_ip[0] << 4) | (source_ip[1] << 3) | (source_ip[2] << 2) | source_ip[1];
        iphdr->daddr = (dest_ip[0] << 4) | (dest_ip[1] << 3) | (dest_ip[2] << 2) | dest_ip[1];

        iphdr->csum = ip_checksum(iphdr);
    }
    { // see RFC768: https://tools.ietf.org/html/rfc768
        struct udphdr* udphdr = &frame->udphdr;
        udphdr->sport = 1024;
        udphdr->dport = 1024;
        udphdr->ulen = sizeof(frame->udphdr) + payload_len;
        udphdr->sum = 0; // don't care
    }
    unsigned char* buf_end = (unsigned char*) (frame + 1) + payload_len;

    gpio_set_as_output(GPIO_PIN_ETHERNET_TDp);
    gpio_set_as_output(GPIO_PIN_ETHERNET_TDm);
    gpio_set_as_output(42);
    gpio_set_value(42, 1);
    
    int v = 0;
    for (;;) {
        gpio_set_value(GPIO_PIN_ETHERNET_TDp, v); v = !v;
        /* wait(1000); // 1000 -> 1ms, 956Hz */
        /* wait(100); // 100 -> 0.1093ms, 9.188KHz */
        /* wait(1); // 1 -> 5us, 200KHz */

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

        // FIXME: TP_IDL (3 * CLOCK positive)

        // FIXME: NLP
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
