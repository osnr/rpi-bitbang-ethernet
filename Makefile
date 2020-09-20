all: rpi-bitbang-ethernet.bin

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

deploy: rpi-bitbang-ethernet.bin
	raspbootcom /dev/tty.usbserial-0001 $<

localtest:
	cc -Wno-implicit-function-declaration -o rpi-bitbang-ethernet rpi-bitbang-ethernet.c
	./rpi-bitbang-ethernet
