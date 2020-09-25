# rpi-bitbang-ethernet

I recently learned that sending UDP packets over 10BASE-T Ethernet is
actually [not that
complicated](https://www.fpga4fun.com/10BASE-T.html).

After seeing how little code it is on an FPGA, I wanted to reimplement
it in C to understand it better / maybe prototype further ideas. (I'm
capable of thinking in C but not in Verilog, yet. And that fpga4fun
example is pretty dense & inlined; reading it doesn't tell you much
about the protocols. Other examples online are mostly assembly for
constrained platforms and equally dense in their own ways.)

The Pi 4 should be [fast
enough](https://github.com/hzeller/rpi-gpio-dma-demo) to bit-bang
Ethernet comfortably (far above 20MHz), unlike your average
microcontroller.

I don't like doing stuff like this in Linux, and it sort of defeats
the point of doing it from scratch, and it's even harder to nail the
timing if you have interrupts and other processes and a kernel flying
around, so we'll do it on bare metal. It's more fun, anyway.

It seems clear that this approach, a few hundred lines of C that
tightly fits the Ethernet/IP/UDP protocols and pokes at some registers
to toggle GPIO pins, is actually less code and more understandable
than using the actual Ethernet controller on the Pi, which [requires a
full USB
stack](https://www.raspberrypi.org/forums/viewtopic.php?t=36044) :-/

This project is more of a byproduct of me trying to understand
Ethernet & that FPGA example rather than a project that's meant to be
used :-)

## how to use

_Warning_: This is pretty sketchy and out of spec for Ethernet, so you
might fry something. Your Pi, your router, etc. We're sending 3.3V
logic when they say [you shouldn't go above
2.8V](https://www.iol.unh.edu/sites/default/files/knowledgebase/ethernet/10basetmau.pdf#page=11). [Magnetics
stuff](https://networkengineering.stackexchange.com/questions/29927/what-is-the-purpose-of-an-ethernet-magnetic-transformer-and-how-are-they-used)
isolates the device if you use a real Ethernet jack. (I think you can
buy a MagJack, though?) In particular, I think you should probably not
try this with a Power over Ethernet port on the other side.

I've only tested it on the Raspberry Pi 4B with 2GB RAM.

[Cut open an Ethernet cable](https://www.fpga4fun.com/10BASE-T0.html),
or use a breakout board (or MagJack), and connect pins 1 and 2 of the
Ethernet cable to [GPIO pins 20 and 21 on the Pi](https://pinout.xyz/)
(on the bottom right). Connect the other end to your Ethernet switch
(or to a computer directly -- the link says you should crossover and
use pins 3 and 6 in this case, but I haven't found that to be
necessary. Pins 1 and 2 seem fine these days).

Get a microSD card and format it to FAT. Copy `vendor/start4.elf` to
the root of the SD card.

Edit `rpi-bitbang-ethernet.c` and put your computer's IP address and
MAC address in.

```
$ make
```

Copy `rpi-bitbang-ethernet.bin` to the root of the SD card and rename
it to `kernel7l.img`.

Run `nc -ul 1024` in a terminal on your computer and leave it on;
the UDP packet from the Pi will show up here.

Put the SD card in your Pi and power it on. The green ACT LED should
toggle every 2 seconds or so, and you should (usually) see the packet
show up on your computer each time!

## how it works

You really only need to look at rpi-bitbang-ethernet.c and transmit.s.

## development

I recommend getting a bootloader. I used [imgrecv]() and [raspbootcom]().

You could bootload over JTAG, but I didn't set that up until late in
the development process.

## debugging

If you don't see the packet from `nc`, you should:

- check if the light on your Ethernet switch for that port turns
on. if not, even the Normal Link Pulses aren't getting through.

- consider plugging the Ethernet cable directly into your computer and
using Wireshark, so at least anything resembling an Ethernet frame
will show up there for analysis, even if some part of it is messed up
so no one can actually receive it.

(Even if the Ethernet frames don't send at all, Wireshark should show
a flurry of ARP and MDNS packets and stuff from your computer as soon
as the Pi starts sending out reasonable link pulses, where your
computer is trying to figure out what's going on with this newly
connected network. )

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

## see also

- [ethertiny](https://github.com/cnlohr/ethertiny) (Hackaday post:
  [Bit-banging Ethernet On An
  ATTiny85](https://hackaday.com/2014/08/29/bit-banging-ethernet-on-an-attiny85/)
  has a great video with explanation)

- [fpga4fun.com 10BASE-T FPGA
  interface](https://www.fpga4fun.com/10BASE-T.html): possibly the
  best 'hacker's explanation' of Ethernet I found. I only started this
  project after I realized their thing worked on my network (used an
  ECP5 FPGA I had lying around), so I had a known-good example -- if
  you can't get this project working, it might be wise to try theirs

- [IgorPlug-UDP](http://web.archive.org/web/20080202054313/https://www.cesko.host.sk/IgorPlugUDP/IgorPlug-UDP%20(AVR)_eng.htm):
  I didn't really look at this, but it's cited around various places
  if you look for stuff about bit-banging Ethernet.

- [10Base-T Medium Attachment Unit
  PDF](https://www.iol.unh.edu/sites/default/files/knowledgebase/ethernet/10basetmau.pdf):
  very clear explanations of timing, has good diagrams

What's new here: The Pi is fast enough and big enough that we can
write mostly in C and have proper structs for all the network headers,
which results in (I think) much clearer and more educational code,
something you could imagine actually turning into more of a proper (if
hacky and personal-use-only) network stack.

## ideas

Internet!

Receive packets! You need to do more physical analog stuff to make it
safe, but I don't think the digital part would be too bad, since the
Pi is so much faster than the 10BASE-T clock? You could just sit on
the wire and sample really fast until you see a level change.

TCP!

I think it would be cool to make a radically small OS with graphics,
networking, etc, where you just dedicate a core to bit-bang each of
them. All the parts of computing that are interesting and fun without
any of the boring and complicated peripheral setup code.
