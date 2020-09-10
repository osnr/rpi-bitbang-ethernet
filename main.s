// via https://www.raspberrypi.org/forums/viewtopic.php?p=1711433&sid=d31ca925bd9c6520baa04cb11c746718#p1711433

/***********************************************************************
* This code was taken from https://www.cl.cam.ac.uk/projects/raspberrypi/tutorials/os/ok02.html 
* and modified so that it also runs on a Rasperry Pi 4.
* Check out the original site for more information.
* The makefile is the same. The Kernel musst renamed to kernel7l.img

.section .init
.globl _start
_start:

/*
* The Basis GPIO Adresse to r0
*/
ldr r0,=0xFE200000 

/*
* LED is Pin42
* on the GPFSEL4 register Bits 8-6
* 001 = GPIO Pin 42 is an output
*/

mov r1,#1
lsl r1,#6  /* -> 001 000 000 */

/*
*  
* 0x10 GPFSEL4 GPIO Function Select 4
*/
 
str r1,[r0,#0x10]

/* 
* Set the 42 Bit
* 25:0 SETn (n=32..57) 0 = No effect; 1 = Set GPIO pin n.
* 42 - 32 = 10
*/


mov r1,#1
lsl r1,#10

loop$: 

/*
* 0x2C GPCLR1 GPIO Pin Output Clear 1 
*/

str r1,[r0,#0x2C]

/* 
* Waiting
*/

mov r2,#0x3F0000
wait1$:
	sub r2,#1
	cmp r2,#0
	bne wait1$

/*
* 0x20 GPSET1 GPIO Pin Output Set 1 
*/

str r1,[r0,#0x20]

/* 
* Waiting
*/

mov r2,#0x3F0000
wait2$:
	sub r2,#1
	cmp r2,#0
	bne wait2$

/*
* And repeat the whole thing over and over again
*/
b loop$
