all: rpi-bitbang-ethernet.bin

# Q: Why not AArch64?
# A: No real reason that wouldn't work. I don't know it, and I
# would've had to find the toolchain to install.

%.o: %.c
	arm-none-eabi-gcc -g -Wall -Og -std=c99 -fno-omit-frame-pointer -ffreestanding -nostdlib -c $< -o $@
%.o: %.s
	arm-none-eabi-as $< -o $@

rpi-bitbang-ethernet.elf: start.o transmit.o rpi-bitbang-ethernet.o
	arm-none-eabi-ld -nostdlib -T link.ld $^ -o rpi-bitbang-ethernet.elf

rpi-bitbang-ethernet.bin: rpi-bitbang-ethernet.elf
	arm-none-eabi-objcopy rpi-bitbang-ethernet.elf -O binary rpi-bitbang-ethernet.bin

clean:
	rm *.o *.bin *.elf

# You don't really need anything below; it's just for my personal use,
# so I have a real debugger & don't have to constantly swap SD card in
# and out to deploy.

# you should leave this running. it should reconnect even when Pi restarts.
jtag-attach:
	openocd -f helpful-but-not-strictly-necessary/ft232r.cfg -f helpful-but-not-strictly-necessary/rpi4.cfg

# these want jtag-attach to be running in background already:

gdb:
	arm-none-eabi-gdb rpi-bitbang-ethernet.elf -ex 'target remote :3333'

deploy: rpi-bitbang-ethernet.bin
# based on https://yeah.nah.nz/embedded/pi-jtag-u-boot/ 
# https://git.nah.nz/pi-zero-jtag/tree/load-u-boot.sh
# I added the `mon reset init`. assumes SRST->Pi's RUN pin
	arm-none-eabi-gdb -ex 'set confirm off' -ex 'target remote :3333' \
		-ex 'mon reset init' \
		-ex 'mon halt' \
		-ex 'mon load_image rpi-bitbang-ethernet.bin 0x8000 bin' \
		-ex 'mon resume 0x8000' \
		-ex 'quit'
