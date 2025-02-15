# rpi-bitbang-ethernet

I recently learned that sending UDP packets over 10BASE-T Ethernet is
actually [not that
complicated](https://www.fpga4fun.com/10BASE-T.html), so I wanted to
try it.

![doc/hello.gif](doc/hello.gif)

In particular, after seeing how little code it is on an FPGA, I wanted
to reimplement it in C to understand it better / maybe ultimately
prototype more ideas / prototype better ways to represent it. (I'm
capable of thinking in C but not in Verilog, yet. And that fpga4fun
example is pretty dense & inlined; reading the code doesn't tell you
much about the protocols. Other examples online are mostly assembly
for constrained platforms, equally dense in their own ways.)

The Raspberry Pi 4 is [fast
enough](https://github.com/hzeller/rpi-gpio-dma-demo) to bit-bang
Ethernet comfortably (far above 20MHz).

I don't like doing stuff like this in Linux, and it sort of defeats
the point of doing it from scratch, and it's even harder to nail the
timing if you have interrupts and other processes and a kernel flying
around, so we'll do it on bare metal. It's more fun, anyway.

It seems clear that this approach, a few hundred lines of
[C](rpi-bitbang-ethernet.c) and [ARM assembly](transmit.s) that
tightly fits the Ethernet/IP/UDP protocols and pokes at some registers
to toggle GPIO pins, is actually less total code and more
understandable than using the actual Ethernet port on the Pi, which
[requires a full USB
stack](https://www.raspberrypi.org/forums/viewtopic.php?t=36044) :-/

This project is more of a byproduct of me trying to understand
Ethernet & that FPGA example rather than a project that's meant to be
_used_ :-) It also doubles as a simple-ish template for bare-metal
programming for the Pi 4. (see also
[valvers.com](https://www.valvers.com/open-software/raspberry-pi/bare-metal-programming-in-c-part-1/))
I expect I'll use it as a starter for other stuff and as documentation
so my future self can relearn how I did these things.

## how to use

_Warning_: All sources say this is pretty sketchy and out of spec for
Ethernet, so you might fry something: your Pi, your router, etc. We're
sending 3.3V logic when they say [you shouldn't go above
2.8V](https://www.iol.unh.edu/sites/default/files/knowledgebase/ethernet/10basetmau.pdf#page=11). [Magnetics
stuff](https://networkengineering.stackexchange.com/questions/29927/what-is-the-purpose-of-an-ethernet-magnetic-transformer-and-how-are-they-used)
isolates the device if you use a real Ethernet jack. (I think you
could buy a MagJack on its own? Maybe that'd be sufficient.) In
particular, I think you should not do this with a Power over Ethernet
port on the other side.

I've only tested this on a single **Raspberry Pi 4B with 2GB RAM**. (I
don't see why it wouldn't work on other Pi 4B models; it might even be
workable on the Pi 2 or 3, although you'd need to mess a lot with
the timings there and change all the 0xFE... memory addresses.)

You'll need the [gcc-arm-none-eabi
toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads)
installed on your computer.

1. [Cut open an Ethernet
cable](https://www.fpga4fun.com/10BASE-T0.html) (or use a breakout
board), and connect pins 1 (green-and-white) and 2 (plain green) of
the Ethernet cable to [GPIO pins 20 and 21 on the
Pi](https://pinout.xyz/) (on the bottom right), respectively.

<img src="doc/cable.jpg" width="400">

2. Get a microSD card and format it to FAT. Copy the firmware
[`start4.elf`](https://github.com/raspberrypi/firmware/blob/master/boot/start4.elf)
to the root of the SD card. (I think this is the
[only](https://www.raspberrypi.org/documentation/configuration/boot_folder.md)
file, other than our binary, that you need on the card on the Pi 4,
unless you want to use JTAG or get more RAM or clock speed or
something.)

3. Edit `rpi-bitbang-ethernet.c` and put your computer's IP address and
MAC address in.

4. Compile:

```
$ make
```

5. Copy `rpi-bitbang-ethernet.bin` to the root of the SD card and rename
it to `kernel7l.img`.

6. Run `nc -ul 1024` in a terminal on your computer and leave it on;
the UDP packet from the Pi should show up here after the next step.

7. Put the SD card in your Pi and power it on. If the program is
running OK, the green ACT LED should toggle every 2 seconds or so.

![doc/actled.gif](doc/actled.gif)

8. Connect the other end of the cable to your Ethernet switch (or to a
computer directly -- the link says you should crossover and use pins 3
and 6 in this case, but I haven't found that to be necessary with
modern hardware. Pins 1 and 2 seem fine).

   Now you should (usually) see the packet show up on your computer
each time the green LED toggles! I have Wireshark open here, so you
can see how the whole Ethernet frame looks:

![doc/wireshark.gif](doc/wireshark.gif)

   Check out, for instance, how the source MAC address is
00:12:34:56:78:90, a value I basically made up and [stuck in the
code](rpi-bitbang-ethernet.c).

## how it works

You really only need to look at
[rpi-bitbang-ethernet.c](rpi-bitbang-ethernet.c) and
[transmit.s](transmit.s). I construct a buffer containing the proper
Ethernet, IP, and UDP headers and the payload, then toggle the GPIO
pins with the right timing to transmit it.

You might find [writeup on development/debugging techniques &
tools](helpful-but-not-strictly-necessary/DEVELOPMENT.md) interesting.

## license

MIT

## see also

- [ethertiny](https://github.com/cnlohr/ethertiny) ([Bit-banging Ethernet On An
  ATTiny85](https://hackaday.com/2014/08/29/bit-banging-ethernet-on-an-attiny85/)
  has a great video with explanation)

- [fpga4fun.com 10BASE-T FPGA
  interface](https://www.fpga4fun.com/10BASE-T.html): the best
  'hacker's explanation' of Ethernet I found. I only started this
  project after I realized their thing worked on my network (used an
  ECP5 FPGA I had lying around), so I had a known-good example -- if
  you can't get this project working, it might be wise to check that
  theirs works

- [IgorPlug-UDP](http://web.archive.org/web/20080202054313/https://www.cesko.host.sk/IgorPlugUDP/IgorPlug-UDP%20(AVR)_eng.htm):
  I didn't really look at this, but it's cited around various places
  if you look for stuff about bit-banging Ethernet.

- [10Base-T Medium Attachment Unit
  PDF](https://www.iol.unh.edu/sites/default/files/knowledgebase/ethernet/10basetmau.pdf):
  very clear explanations of timing, has [good
  diagram](https://www.iol.unh.edu/sites/default/files/knowledgebase/ethernet/10basetmau.pdf#page=8)

What's new here vs. these projects, maybe: The Pi is fast enough and
big enough that we can write mostly in C and have proper structs for
all the network headers, which results in (I think) much clearer and
more educational code, something you could imagine actually turning
into a coherent (if hacky and personal-use-only) network stack.

## ideas

(vague)

Make the packet vary! Based on GPIO pin input? It is a little sad to
just send a fixed packet. It feels like I'm betraying the spirit of
the programmable computer, which is supposed to take input and change
over time. This thing might as well be a fixed circuit.

~Internet! Maybe this would just work out of the box?~ it does work!
it sends fine over Internet :D

Receive packets! You might need to do more physical [analog
stuff](https://www.fpga4fun.com/10BASE-T4.html) to make it work, but I
don't think the digital part would be too bad, since the Pi is so much
faster than the 10BASE-T clock? You could just sit on the wire and
sample really fast and look for level changes.

I wonder if you could run this in Linux by dedicating a core to
it. Could you get it to interface with the OS and act as a NIC? Might
be a fun way to learn about driver development.

["Could you make a working DHCP server by soldering wires from a CAT5 cable straight onto an ICE40 FPGA?"](https://twitter.com/lukego/status/1248306300615868419) / ["You can do 100base-TX with a Spartan-6 and a dozen resistors"](https://twitter.com/azonenberg/status/1248308397994151939)

TCP!

## hmm

I guess there's something that I feel drawn to about using a tiny
amount of code to interoperate with a giant, complicated, modern
system -- because the system still understands the dead simple 1970s
protocol at heart.

I also like the idea of leapfrogging what would have had to be custom
hardware in the old days using the combination of simple code and an
overkill modern CPU.

I think it would be cool to make a radically small OS with graphics,
networking, and so on, where you just dedicate cores to bit-bang each
of them. All the parts of computing that are interesting and fun
without any of the boring and arbitrary peripheral setup code / the
essential complexity of protocols instead of the inessential
complexity of drivers
