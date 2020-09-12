SECTIONS
{
    . = 0x8000;
    .text : {
          start.o (.text)
          *(.text*)
    }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : {
        __bss_start = .;
        *(.bss*)
        *(COMMON)
    }
    . = ALIGN(8);
    __bss_end = .;
}
