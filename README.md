# pi-ethernet-bitbang

I'm not that interested in Linux, so we'll do it on bare metal.

## how to use

I've only tested it on the Raspberry Pi 4B with 2GB RAM.

Get a microSD card and format it to FAT. Copy `vendor/start4.elf` to
the root of the SD card.

```
$ make
```

Copy `rpi-bitbang-ethernet.bin` to the root of the SD card and rename
it to `kernel7l.img`.

Put the SD card in your Pi and turn it on. It should blink.

<!-- cut open an ethernet cable -->
