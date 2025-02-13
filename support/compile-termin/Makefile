BIN=/home/ubuntu16/emacsimdir/bin

UTIL=/home/cs580000/share/hoca/util

GCC=$(BIN)/m68k-elf-gcc
GCC_FLAGS=-m68000 -I$(UTIL)/inc/ -I$(UTIL)/inc/m68k-linux-gnu -g -gstabs -c

LD=$(BIN)/m68k-elf-ld
LD_FLAGS=-T script.x -L$(UTIL)/lib -L$(BIN)/../lib/gcc/m68k-elf/4.8.2/m68000

LIBS=-lutil -lc -lsim -lm -lgcc

CRT0=crt0.o

STRIPH=./striph


all: termin0


clean:
	rm -f t0.o t0 termin0


t0.o: t0.c
	$(GCC) $(GCC_FLAGS) -o t0.o t0.c


termin0: t0.o
	$(LD) $(LD_FLAGS) -o t0 $(CRT0) t0.o $(LIBS)
	$(STRIPH) t0 >termin0
