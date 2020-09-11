main.bin: main.s
	arm-none-eabi-as main.s -o main.o
	arm-none-eabi-objcopy main.o -O binary main.bin
