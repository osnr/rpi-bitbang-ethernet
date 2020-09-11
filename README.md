# pi-ethernet-bitbang

I've only tested it on the Raspberry Pi 4B with 2GB RAM.

```
$ make
```

Get a microSD card and format it to FAT.

Copy `vendor/start4.elf` to the root of the SD card.

Copy `main.bin` to the root of the SD card and rename it to
`kernel7l.img`.

Put the SD card in your Pi and turn it on. It should blink.

<!-- cut open an ethernet cable -->
