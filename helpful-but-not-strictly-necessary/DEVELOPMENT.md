## development

## bootloader

I recommend getting some kind of bootloader setup where you don't need
to manually copy each build to the SD card.

For most of this project, I used
[imgrecv](https://gitlab.com/bztsrc/imgrecv) and
[raspbootcom](https://github.com/mrvn/raspbootin). imgrecv was the
only bootloader stub I found (on a cursory search) that clearly
supported the Pi 4.

Some weird issues (both trying to bootload the .elf and the raw .bin).

- The .elf got loaded at 0x10000 or something instead of 0x8000,
so all absolute addresses broke. This meant that the program often
kinda worked, but references to data segments often got totally mangled, so
it'd crash if you tried anything complicated. I think you might need
to build the .elf in some special way to be understood, but eh -- I
like the symmetry of loading the same .bin file that you could copy to
the SD card.

- The .bin got loaded properly at 0x8000, but the bootloader jumped to
0x8000+[some weird offset] instead of 0x8000. I got it to work for a
while by just putting a bunch of NOPs at the top of start.s. Yikes.

I have no idea if these problems are with the imgrecv stub or the
raspbootcom utility, or if I'm just doing something wrong. Maybe they
have to do with this being a Pi 4 and/or this being a 32-bit program? That
seemed like an under-tested code path.

(btw, I only properly diagnosed those issues once I bothered to set up
JTAG and looked at the behavior in GDB. but once I had JTAG, might as
well just bootload through that too, so I didn't care to figure out
the causes here.)

Anyway, I ended up switching to JTAG to do all the bootloader stuff,
which simplifies the wiring as well (one dongle, not two).

I think you could also netboot (you'd need to run a separate Ethernet
cable to the proper Ethernet port). I didn't look into that.

## debugging

If you don't see the packet from `nc`, you should:

- check if the light on your Ethernet switch for that port turns
on. This shows that at least the Normal Link Pulses are getting
through. (My router even turns the light amber instead of green to
show that it's a non-Gigabit connection!)

- consider plugging the Ethernet cable directly into your computer and
using Wireshark, so at least anything resembling an Ethernet frame
will show up there for analysis, even if some part of it is messed up
so no one can actually receive it.

    (Even if the Ethernet frames don't send at all, Wireshark should show
a flurry of ARP and MDNS packets and stuff from your computer as soon
as the Pi starts sending out reasonable link pulses, where your
computer is trying to figure out what's going on with this newly
connected network.)

other useful techniques:

- binary-search debugging. if you make a change & the Pi
  just... doesn't do anything, make the ACT LED turn on at some point
  in your program. if it doesn't turn on, then move that turn-on line
  further up. if it does turn on, you know the program is known-good
  up to that point, so move that line downward. keep doing this until
  you narrow down to the exact point of failure.

- use other GPIO pins to set intervals when things happen + a logic
  analyzer or oscilloscope. I also used this to compute timing once
  individual bits were too fast for my logic analyzer: I'd
  mark the total packet time, measure that, then divide by number of
  bits to make sure I was in line with the 100 ns/bit standard.

- compile rpi-bitbang-ethernet.c for your computer and execute it
  there, especially if you don't want to bother with JTAG. I used to
  have `#ifdef __arm__` for all the Pi-specific stuff (poking GPIO
  registers, etc). the other branch (for when I compiled targeting my
  Mac) would printf all the bytes that I wanted to send. this is nice
  if you're having problems with protocol logic that don't require
  actual hardware to figure out.

- so I often printed out the whole buffer that I wanted to send, then
  pasted it into [Hex Packet Decoder](https://hpd.gasmi.net/) to make
  sure it'd be valid

- slow down timing. my logic analyzer could decode the Manchester
  encoding after I slowed down the timing enough that it could sample
  it properly. (then I could copy the decoded bytes out and paste them
  into Hex Packet Decoder!)

### jtag

I recommend getting a JTAG dongle (I just used an [FT232R
dongle](https://jacobncalvert.com/2020/02/04/jtag-on-the-cheap-with-the-ftdi-ft232r/)
I had lying around; another Pi acting as a PC & running full-fledged
Raspberry Pi OS would probably work, too) and using OpenOCD and
gdb. [This
tutorial](https://metebalci.com/blog/bare-metal-raspberry-pi-3b-jtag/)
covers most of it, including the changes you need for the Pi 4B.

[Reports
vary](https://www.raspberrypi.org/forums/viewtopic.php?t=254142), but
personally, I found that I needed to connect TDI, TCK, TDO, TMS, and
the supposedly optional TRST, but I didn't need to connect RTCK. (and
I couldn't have connected RTCK even if I had needed to, because the
[ft232r bitbang
driver](http://www.openocd.org/doc/html/Debug-Adapter-Configuration.html)
in OpenOCD doesn't seem to support it.)

You also need to put `enable_jtag_gpio=1` in config.txt on the SD
card, or mess around with the GPIO pin functions manually in the
program. Also make sure that you connect all the JTAG wires to the
[alt4 JTAG pins](https://pinout.xyz/pinout/jtag) on the Pi; I wasted a
lot of time because I had wires connected to a mix of alt4 and alt5.

Also solder a header onto the RUN pin on the Pi and connect that to
SRST if you want to be able to [auto-reset](ft232r.cfg) the Pi
whenever you boot-load a new program.

## tools

You don't _strictly_ need all of these -- which is why they're not
mentioned in the main README -- but it is nice to have some margin for
error and to be able to see what is going wrong (and what is going
_right_, which builds confidence in your progress..).

- logic analyzer (Saleae Logic 4)

- FT232R dongle: for JTAG. could also use other (better) JTAG dongle,
  or another Pi

- CP2102 dongle: for bootloader

- a lot of jumpers: to connect to Ethernet

- header: solder onto RUN pin for reset

- Pi 4

- microSD card

- microSD card reader

- Ethernet-to-USB-C adapter
