ENTRY (_start)

SECTIONS
{
	. = 0x80000;
	.text : { *(.text) }
	.data : { *(.text) }
	.bss : { *(.text) }
}
