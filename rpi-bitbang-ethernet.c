#include "util.h"
#include "checksums.h"

#define PIN_ETHERNET_TDp  20
#define PIN_ETHERNET_TDm  21

#define PIN_ACT_LED 42

// Warning: Any 2- and 4-byte fields in these headers are expected to
// be filled as big-endian by the time you send the packet, while
// (unless you're doing some really weird stuff) the Raspberry Pi's
// ARM CPU is little-endian.

// It would probably be better to make explicit, compiler-checkable BE
// struct types & helper functions, but right now, I just hack around
// it by flipping bytes after assignment in main() where needed...

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

struct set_clr_pins { unsigned int set_pins; unsigned int clr_pins; };

extern void wait(int n);
extern void transmit_from_set_clr_pins_buf(unsigned int target_pin_ethernet_tdp,
                                           unsigned int target_pin_ethernet_tdm,
                                           struct set_clr_pins* set_clr_pins_buf,
                                           struct set_clr_pins* set_clr_pins_buf_end);
extern void normal_link_pulse(unsigned int target_pin_ethernet_tdp,
                              unsigned int target_pin_ethernet_tdm);

void transmit(unsigned char* buf, int buflen);

void main(void) {
    // I was hoping I wouldn't need this, but it turned out to be
    // pretty necessary. This utility function sets up the MMU and
    // enables the instruction and data caches, which makes everything
    // about 70x faster, which is fast enough that we can get OK
    // timing to bit-bang Ethernet (30 NOPs ~= 50 nanoseconds; without
    // this stuff, just 1-2 instructions take 50 ns, so the timing is
    // totally unreliable and asymmetrical...)
    enable_mmu();
    // I think we could probably get even faster and more reliable
    // (~2x?) if we configured the clock speed? I didn't look into that.
    // see this thread: https://www.raspberrypi.org/forums/viewtopic.php?t=219212
    // & this project: https://github.com/hzeller/rpi-gpio-dma-demo
    // (that runs on Linux where all the perf stuff is already enabled)

    const unsigned char source_ip[] = {192, 168, 1, 44}; // I made this up! change it!

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
        // (the proper Internet-y way to do this would be to do an ARP
        // thing to online resolve MAC addr from IP addr, instead of
        // hard-coding MAC ?)
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

        iphdr->csum = ip_checksum(iphdr, sizeof(*iphdr));
    }
    { // see RFC768: https://tools.ietf.org/html/rfc768
        struct udphdr* udphdr = &frame->udphdr;
        udphdr->sport = 1024; udphdr->sport = (udphdr->sport>>8) | (udphdr->sport<<8);
        udphdr->dport = 1024; udphdr->dport = (udphdr->dport>>8) | (udphdr->dport<<8);
        udphdr->ulen = sizeof(frame->udphdr) + payload_len; udphdr->ulen = (udphdr->ulen>>8) | (udphdr->ulen<<8);
        udphdr->sum = 0; // don't care
    }
    unsigned char* buf_end = (unsigned char*) (frame + 1) + payload_len;
    
    struct frametlr* frametlr = (struct frametlr*) buf_end;
    frametlr->fcs = crc32b((unsigned char*) frame, buf_end - (unsigned char*) frame);
    
    buf_end += sizeof(*frametlr);

    gpio_set_as_output(PIN_ETHERNET_TDp);
    gpio_set_as_output(PIN_ETHERNET_TDm);

    gpio_set_as_output(PIN_ACT_LED); // green LED just so you can see when it sends

    gpio_set_value(PIN_ETHERNET_TDp, 0);
    gpio_set_value(PIN_ETHERNET_TDm, 0);

    int v = 0;

    int nlps_sent = 0;
    for (;;) {
        wait(75000 * 70); // ~16ms

        if (++nlps_sent % 125 == 0) { // send packet ~every 2 seconds
            gpio_set_value(PIN_ACT_LED, (v = !v));
            transmit(buf, buf_end - buf);

        } else {
            normal_link_pulse((1 << PIN_ETHERNET_TDp), (1 << PIN_ETHERNET_TDm));

            // Once the Normal Link Pulse is working, the little
            // status LED on your switch/router should at least light
            // up for the Ethernet port that the Pi is connected to.

            // If you're watching in Wireshark and the Pi is plugged
            // straight into your computer or something, you should
            // also see the network interface go from silence to a
            // patter of ARP / other weird discovery packets as your
            // computer sees that there's something there.
        }

        // see https://www.fpga4fun.com/10BASE-T3.html
    }
}

void transmit(unsigned char* buf, int buflen) {
    // A sequence where each item in the sequence says 'set these pins
    // set_pins and clear these pins clr_pins'.
    struct set_clr_pins set_clr_pins_buf[(buflen * 8) * 2];
    
    // Each item in the sequence will be enacted and held for a
    // half-bit-time. So the assembly-language transmission routine
    // that we call won't know anything about Manchester encoding;
    // it'll just go through each item in the sequence, follow our
    // precomputed instructions of what pins to assign what values,
    // then wait, then assign the next set of pins according to the
    // next item in the sequence, and so on. The encoding is all done
    // ahead of time in this C function.

    // I wonder if you could just emit machine code directly here...
    // (that's what the AVR one does, although he does the compilation
    // offline: https://github.com/cnlohr/ethertiny/tree/master/t85)

    int k = 0;
    // Go through each bit in buf & turn it into 2 items in the
    // sequence (the two half-bits of the 10BASE-T Manchester
    // encoding)
    for (int i = 0; i < buflen; i++) {
        for (int j = 0; j < 8; j++) {
            int bit = (buf[i] >> j) & 1;
            if (bit) { // LOW => HIGH
                set_clr_pins_buf[k].set_pins = 1 << PIN_ETHERNET_TDm;
                set_clr_pins_buf[k++].clr_pins = 1 << PIN_ETHERNET_TDp;

                set_clr_pins_buf[k].set_pins = 1 << PIN_ETHERNET_TDp;
                set_clr_pins_buf[k++].clr_pins = 1 << PIN_ETHERNET_TDm;

            } else { // HIGH => LOW
                set_clr_pins_buf[k].set_pins = 1 << PIN_ETHERNET_TDp;
                set_clr_pins_buf[k++].clr_pins = 1 << PIN_ETHERNET_TDm;

                set_clr_pins_buf[k].set_pins = 1 << PIN_ETHERNET_TDm;
                set_clr_pins_buf[k++].clr_pins = 1 << PIN_ETHERNET_TDp;
            }

            // (By the way, it actually works... sometimes... if you
            // just leave TD- at 0 and only toggle TD+. I was doing
            // that for a while. I found that my laptop with USB
            // Ethernet adapter accepted packets sent this way, and my
            // external Ethernet switch did too, but my Wi-Fi router
            // did not. Better to follow the spec as closely as we can
            // [...not very])
        }
    }

    transmit_from_set_clr_pins_buf((1 << PIN_ETHERNET_TDp), (1 << PIN_ETHERNET_TDm),
                                   set_clr_pins_buf, &set_clr_pins_buf[k]);
}
