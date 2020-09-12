#include "rpi-utils.h"

#define TDp 1
#define TDm 2

const unsigned int source_ip[] = {192, 168, 1, 44};

const unsigned int dest_ip[] = {192, 168, 1, 6};
const unsigned int dest_phys[] = {0x78, 0x4F, 0x43, 0x88, 0x3B, 0xE2};

void sleep(int n) {
    // 0x3F0000 = about 3 seconds?
    for (volatile int i = 0; i < n; i++) {}
}

void main(void) {
    unsigned char frame[256];

    __asm__(
            "ldr r0,=0xFE200000   \n"
            "mov r1, #1           \n"
            "lsl r1,#6            \n"  /* -> 001 000 000 */
            "str r1,[r0,#0x10]    \n"
            "mov r1,#1            \n"
            "lsl r1,#10           \n"

            "str r1,[r0,#0x2C]  \n"
            : : : "r0","r1" );

    for (;;) {
        unsigned int ip_checksum = 
            0x0000C53F + (source_ip[0] << 8) + source_ip[1] + (source_ip[2] << 8) + source_ip[3] +
            (dest_ip[0] << 8) + dest_ip[1] + (dest_ip[2] << 8) + dest_ip[3];
        ip_checksum = (ip_checksum & 0x0000FFFF) + (ip_checksum >> 16);
        ip_checksum = (ip_checksum & 0x0000FFFF) + (ip_checksum >> 16);

        __asm__(
            "ldr r0,=0xFE200000   \n"
            "mov r1,#1            \n"
            "lsl r1,#10           \n"

            "str r1,[r0,#0x2C]  \n"
            : : : "r0","r1" );
        
        sleep(0x3F0000);

        __asm__(
            "ldr r0,=0xFE200000   \n"
            "mov r1,#1            \n"
            "lsl r1,#10           \n"

            "str r1,[r0,#0x20]  \n"
            : : : "r0","r1" );

        sleep(0x3F0000);
    }
}
